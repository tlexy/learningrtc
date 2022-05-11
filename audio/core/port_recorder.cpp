#include "port_recorder.h"
#include "iport_callback.h"
#include "file_saver.h"

#pragma execution_character_set("utf-8")

#define PORT_SAMPLES_PER_FRAME 1024
#define PORT_INPUT_TEMP (PORT_SAMPLES_PER_FRAME * 2 * 4)
//#define TEST_SAVE_DUMP

std::map<int, const PaDeviceInfo*> PortRecorder::device_list;
int PortRecorder::_recorder_id_s = 10000;

PortRecorder::PortRecorder(IPortCallBack* cb)
	:_cb(cb),
	_record_thread(std::shared_ptr<std::thread>()),
	_swr_ctx(NULL),
	_is_record_stop(true),
	_error_count(0),
	_sample_size(0),
	_ns(NULL),
	_is_anc_start(false),
	_ns_swr_ctx(NULL)
{
	_recorder_id = _recorder_id_s++;

	_ns_swr_buf_size = 4096;
	_ns_swr_buf = new char[_ns_swr_buf_size];

	/*_ns_prebuf_size = 4096 * 2;
	_ns_prebuf_pos = 0;
	_ns_prebuf = new char[_ns_prebuf_size];
	_ns_temp_buf = new char[1024];*/
	_ns_pre_mid = new mid_buf(1024 * 1024);

	_ns_after_size = 1024*1024;
	_ns_after_pos = 0;
	_ns_afterbuf = new char[_ns_after_size];
	_ns_afterbuf_temp_len = 1024 * 10;
	_ns_afterbuf_temp = new char[_ns_afterbuf_temp_len];

	_target_swr_ctx = init_swr(32000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);

	_level_anc.push_back(new NoiseSuppresion(1, 32000));
	_level_anc.push_back(new NoiseSuppresion(2, 32000));
	_level_anc.push_back(new NoiseSuppresion(3, 32000));
}

PortRecorder::~PortRecorder()
{
	if (_ns)
	{
		delete _ns;
		_ns = NULL;
	}
	if (_ns_swr_buf)
	{
		delete[] _ns_swr_buf;
		_ns_swr_buf = NULL;
	}
	/*if (_ns_prebuf)
	{
		delete[] _ns_prebuf;
		_ns_prebuf = NULL;
	}
	if (_ns_temp_buf)
	{
		delete[] _ns_temp_buf;
		_ns_temp_buf = NULL;
	}*/
	if (_ns_afterbuf)
	{
		delete[] _ns_afterbuf;
		_ns_afterbuf = NULL;
	}
	if (_ns_afterbuf_temp)
	{
		delete[] _ns_afterbuf_temp;
		_ns_afterbuf_temp = NULL;
	}
}

struct SwrContext* PortRecorder::init_swr(int src_rate, int src_channels, int src_fmt, int dst_rate, int dst_channels, int dst_fmt)
{
	struct SwrContext* swr_ctx;
	/* create resampler context */
	swr_ctx = swr_alloc();
	/* set options */
	av_opt_set_int(swr_ctx, "in_channel_layout", src_channels, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", src_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (AVSampleFormat)src_fmt, 0);

	av_opt_set_int(swr_ctx, "out_channel_layout", dst_channels, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", dst_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", (AVSampleFormat)dst_fmt, 0);

	int ret = 0;
	/* initialize the resampling context */
	if ((ret = swr_init(swr_ctx)) < 0)
	{
		fprintf(stderr, "Failed to initialize the resampling context\n");
		return NULL;
	}
	return swr_ctx;
}

int PortRecorder::do_swr(struct SwrContext* swr, void* in_data, int nb_in_samples, int src_rate, void* out_data, int out_len, int dst_rate)
{
	int dst_nb_samples = av_rescale_rnd(nb_in_samples, dst_rate, src_rate, AV_ROUND_UP);
	if (dst_nb_samples * 2 * 2 > out_len)
	{
		//空间太小，无法重采样
		fprintf(stderr, "Too small buffer for ouput port recorder\n");
		return -1;
	}
	static uint8_t* out_buf[1];
	const static uint8_t* in_buf[1];
	out_buf[0] = (uint8_t*)out_data;
	in_buf[0] = (uint8_t*)in_data;
	int num = swr_convert(swr, &out_buf[0], dst_nb_samples, &in_buf[0], nb_in_samples);
	//fprintf(stderr, "dst_nb_samples:real_out_sample, %d:%d\n", dst_nb_samples, num);
	return num;
}

bool PortRecorder::startRecord(int paDeviceIndex)
{
	log_w("start record, idx: %d", paDeviceIndex);
	_src_rate = 0;
	if (device_list.find(paDeviceIndex) == device_list.end())
	{
		log_e("port recorder, device index not exist: %d", paDeviceIndex);
		return false;
	}
	_src_rate = device_list[paDeviceIndex]->defaultSampleRate;
	log_w("start record, idx: %d, src_rate: %d", paDeviceIndex, _src_rate);
	if (_src_rate < 44100)
	{
		log_e("port recorder, sample rate too small: %d", _src_rate);
		return false;
	}
	if (_record_thread)
	{
		log_e("port recorder, thread is running");
		return false;
	}
	//当前采样是否支持？
	_swr_ctx = NULL;

	_frame_size = 1024;
	if (_src_rate == 48000)
	{
		_frame_size = 1115;
	}
	_ns_cache_count = 0;
	_is_record_stop = false;
	_record_thread = std::make_shared<std::thread>(&PortRecorder::thread_record, this, paDeviceIndex);
	return true;
}

void PortRecorder::startANC(int mode)
{
	if (mode < 1 || mode > 3)
	{
		return;
	}
	if (_level_anc.size() < mode || _level_anc[mode - 1] == NULL)
	{
		return;
	}
	log_i("start ANC, mode: %d", mode);
	_ns = _level_anc[mode - 1];
	_ns->reset();

	/*if (_ns_swr_ctx != NULL)
	{
		swr_free(&_ns_swr_ctx);
		_ns_swr_ctx = NULL;
	}*/
	//???
	_ns_swr_ctx = init_swr(_src_rate, AV_CH_LAYOUT_STEREO, _src_fmt, 32000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
	
	if (_ns != NULL)
	{
		_is_anc_start = true;
	}
	else
	{
		_is_anc_start = false;
	}

}

bool PortRecorder::needANC()
{
	if (_is_anc_start && _ns_swr_ctx != NULL && _ns != NULL)
	{
		return true;
	}
	return false;
}

static FileSaver* fs320 = NULL;
static FileSaver* fs_ns320 = NULL;

//framecount总为1024
int PortRecorder::doNsSwr(const char* buf, int frameCount, char* out_buf, int outlen)
{
	//qDebug() << "doNsSwr: frameCount" << frameCount;
	//重采样
	int count = do_swr(_ns_swr_ctx, (char*)buf, frameCount, _src_rate, _ns_swr_buf, _ns_swr_buf_size, 32000);
	//
#ifdef TEST_SAVE_DUMP
	if (fs320 == NULL)
	{
		fs320 = new FileSaver(32000 * 2 * 2 * 30, "test320.pcm");
	}
	else
	{
		fs320->write(_ns_swr_buf, count * 2 * 2);
	}
#endif
	//
	//qDebug() << "doNsSwr: swr1, count: " << count;
	//count~~743, 2972~
	//降噪，每640取一个
	_ns_pre_mid->push((const uint8_t*)_ns_swr_buf, count * 2 * 2);
	int a_size = _ns_pre_mid->usable_count();
	while (a_size >= _ns_step && _is_record_stop == false)
	{
		if (_ns_pre_mid->usable_count() >= _ns_step)
		{
			_ns->process_ns_stereo((const char*)_ns_pre_mid->data(), _ns_step);
			a_size -= _ns_step;
			_ns_pre_mid->set_read(_ns_step);
		}
		else
		{
			break;
		}
	}

	//剩余的空间太小
	if (_ns_after_size - _ns_after_pos < _ns_frame_size * 2 * 2)
	{
		if (_ns_afterbuf_temp_len < _ns_after_pos - _ns_after_read_pos)
		{
			delete[] _ns_afterbuf_temp;
			_ns_afterbuf_temp_len = _ns_after_pos - _ns_after_read_pos;
			_ns_afterbuf_temp = new char[_ns_afterbuf_temp_len];
		}
		memcpy(_ns_afterbuf_temp, _ns_afterbuf + _ns_after_read_pos, _ns_after_pos - _ns_after_read_pos);
		memcpy(_ns_afterbuf, _ns_afterbuf_temp, _ns_after_pos - _ns_after_read_pos);
		_ns_after_pos = _ns_after_pos - _ns_after_read_pos;
		_ns_after_read_pos = 0;
	}

	//取降噪后的数据, 全部取完，取的位数必须为偶数
	//int get_count = _ns_after_size - _ns_after_pos;
	int acount = 0;
	if (!_is_record_stop)
	{
		acount = _ns->get_process_data2(_ns_afterbuf + _ns_after_pos, _ns_after_size - _ns_after_pos);
	}
#ifdef TEST_SAVE_DUMP
	if (fs_ns320 == NULL)
	{
		fs_ns320 = new FileSaver(32000 * 2 * 2 * 30, "test_ns320.pcm");
	}
	else
	{
		fs_ns320->write(_ns_afterbuf + _ns_after_pos, acount);
	}
#endif
	_ns_after_pos += acount;
	//存了N帧之后，再进行输出
	++_ns_cache_count;
	if (_ns_cache_count <= _ns_cache_limit)
	{
		return 0;
	}
	int use = 0;
	//再次重采样，采样格式为发送需要的格式
	int after_sample_size = _ns_frame_size * 2 * 2;
	if (_ns_after_pos - _ns_after_read_pos >= after_sample_size)
	{
		int u_count = do_swr(_target_swr_ctx, _ns_afterbuf + _ns_after_read_pos, _ns_frame_size, 32000, out_buf, outlen, _dst_rate);
		if (u_count > 0)
		{
			use = u_count;
			_ns_after_read_pos += (after_sample_size);
		}
		else
		{
			log_w("something maybe wrong when swr from 32000, u_count: %d", u_count);
		}
	}
	else
	{
		return 0;
	}
	//qDebug() << "----------------------------------------return use: " << use << "\tafter pos:" << _ns_after_pos;
	return use;
}

static char port_input_temp_buff[PORT_INPUT_TEMP];//最大float32
static char port_swr_buff[PORT_INPUT_TEMP*2];//最大float32

static FileSaver* fs_send = NULL;

//static int port_off = 0;
//static char pcm_port_buff[1024 * 4 * 2 * 43 * 50];//50s的原始数据
//static bool port_is_save = false;

static FileSaver* fs_raw = NULL;

int port_record_cb(
	const void* input, void* output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	PortRecorder* recorder = (PortRecorder*)userData;
#ifdef LOOP_BACK_PRINT_TEST
	if (frame_time == 0)
	{
		frame_time = SUtil::getTimeStampMilli();
	}
	frame_stat += frameCount;
	if (frame_stat >= 44100)
	{
		int64_t now = SUtil::getTimeStampMilli();
		log_w("record cd, frame count: %d, time diff: %lld", frame_stat, now - frame_time);
		frame_time = now;
		frame_stat = 0;
	}
#endif
	if (recorder->_is_record_stop)
	{
		log_w("port recorder, recorder callback complete.");
		return paComplete;
	}
	int framesize = frameCount * recorder->_sample_size * 2;
	if (framesize > PORT_INPUT_TEMP)
	{
		framesize = PORT_INPUT_TEMP;
		++recorder->_error_count;
		if (recorder->_error_count == 1)
		{
			log_w("port recorder, port framesize too big. %d", framesize);
		}
		if (recorder->_error_count >= 100)
		{
			recorder->_error_count = 0;
		}
	}

	memcpy(port_input_temp_buff, input, framesize);

	//
#ifdef TEST_SAVE_DUMP
	if (fs_raw == NULL)
	{
		fs_raw = new FileSaver(44100 * 2 * 2 * 30, "test_raw.pcm");
	}
	else
	{
		fs_raw->write(port_input_temp_buff, framesize);
	}
#endif
	//qDebug() << "port_record_cb, count: " << frameCount;
	//
	if (recorder->_is_anc_start == false && _G.g_ns_level >= 0 && recorder->_need_update_ns == true)
	{
		recorder->ChangeANC();
	}
	if (recorder->needANC())
	{
		memset(port_swr_buff, 0x0, sizeof(port_swr_buff));
		int count = recorder->doNsSwr(port_input_temp_buff, frameCount, port_swr_buff, sizeof(port_swr_buff));
		//qDebug() << "port_record_cb doNsSwr, count: " << count;
		if (count < 1024 && count > 1000)
		{
			int fix = (1024 - count) * 2 * 2;
			memcpy(port_swr_buff + count * 2 * 2, port_swr_buff + count * 2 * 2 - fix, fix);
		}
		count = 1024;
		if (count > 0)
		{
#ifdef TEST_SAVE_DUMP
			if (fs_send == NULL)
			{
				fs_send = new FileSaver(44100 * 2 * 2 * 30, "test_snd.pcm");
			}
			else
			{
				fs_send->write(port_swr_buff, count * 2 * 2);
			}
#endif
			recorder->_cb->stream_cb(port_swr_buff, count, 16);
			return paContinue;
		}
		else
		{
			//当降噪返回0时，会走没有降噪的流程（如果是有中间发生这种事情，可能会导致声音不一致，因为降噪有大概两帧（？）的缓存）
			log_w("ANC get null.");
		}
	}
	//else

	{
		//重采样
		if (recorder->_swr_ctx != NULL)
		{
			//qDebug() << "swr 1, input frame: " << framesize;
			int count = recorder->do_swr(recorder->_swr_ctx, port_input_temp_buff, frameCount, recorder->_src_rate,
				port_swr_buff, sizeof(port_swr_buff), recorder->_dst_rate);
			//qDebug() << "swr 2, count: " << count;
			recorder->_cb->stream_cb(port_swr_buff, count, 16);
			//qDebug() << "swr 3 ";
		}
		else
		{
			//不需要重采样，直接复制
			recorder->_cb->stream_cb(port_input_temp_buff, frameCount, 16);
		}
	}

	return paContinue;
}

void PortRecorder::thread_record(int index)
{
	log_w("port recorder, recorder thread start, recorder id: %d", _recorder_id);
	const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(index);
	if (!dinfo)
	{
		log_w("port recorder, thread_record not start, index not exist: %d", index);
		return;
	}

	PaStream* inputStream;
	PaStreamParameters inputParam;
	inputParam.channelCount = 2;
	inputParam.device = index;
	inputParam.sampleFormat = paInt16;// paFloat32;

	inputParam.hostApiSpecificStreamInfo = NULL;
	inputParam.suggestedLatency = dinfo->defaultHighInputLatency;//dinfo->defaultLowInputLatency;
	if (inputParam.sampleFormat == paFloat32)
	{
		_sample_size = 4;
	}
	else
	{
		_sample_size = 2;
	}
	PaError err = Pa_IsFormatSupported(&inputParam, NULL, dinfo->defaultSampleRate);
	if (err != paNoError)
	{
		_sample_size = 2;
		log_e("port recorder, format not support, index: %d, name: %s, sampleFormat: %d, sampleRate: %d", index, dinfo->name, inputParam.sampleFormat, dinfo->defaultSampleRate);
		log_e("port recorder, try 16bit");
		inputParam.sampleFormat = paInt16;
		err = Pa_IsFormatSupported(&inputParam, NULL, dinfo->defaultSampleRate);
		if (err != paNoError)
		{
			log_e("port recorder, try 16bit failed.");
		}
	}
	if (err != paNoError)
	{
		log_e("port recorder, format not support, thread stop.");
		return;
	}
	if (inputParam.sampleFormat == paInt16 && dinfo->defaultSampleRate == 44100)
	{
		_swr_ctx = NULL;
	}
	else
	{
		AVSampleFormat src_fmt = AV_SAMPLE_FMT_FLT;
		if (inputParam.sampleFormat == paInt16)
		{
			src_fmt = AV_SAMPLE_FMT_S16;
		}
		_src_fmt = src_fmt;
		_swr_ctx = init_swr(_src_rate, AV_CH_LAYOUT_STEREO, src_fmt, _dst_rate, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
	}
	log_w("port recorder open stream: index: %d, name: %s, sampleFormat: %d, sampleRate: %f", index, dinfo->name, inputParam.sampleFormat, dinfo->defaultSampleRate);
	err = Pa_OpenStream(&inputStream, &inputParam, NULL, dinfo->defaultSampleRate, _frame_size, 0, port_record_cb, this);

	if (err != paNoError)
	{
		log_e("port recorder, open error: %s", Pa_GetErrorText(err));
	}
	ChangeANC();
	err = Pa_StartStream(inputStream);
	if (err != paNoError)
	{
		log_e("start error: ", Pa_GetErrorText(err));
	}
	_stream = inputStream;
}

void PortRecorder::ChangeANC()
{
	if (_G.g_ns_level > 0 && _G.g_ns_level != _ns_level && _G.g_ns_level <= 3)
	{
		_ns_level = _G.g_ns_level;
		startANC(_ns_level);
	}
	if (_G.g_ns_level == 0)
	{
		_ns_level = 0;
		_is_anc_start = false;
	}
	_need_update_ns = false;
}

void PortRecorder::onChangeANC()
{
	_need_update_ns = true;
	_is_anc_start = false;
}

void PortRecorder::stopRecord()
{
	log_w("port recorder, stop record, recorder_id: %d", _recorder_id);
	_is_record_stop = true;
	if (_stream)
	{
		Pa_AbortStream(_stream);
		Pa_StopStream(_stream);
		_stream = NULL;
	}
	if (_record_thread)
	{
		_record_thread->join();
		_record_thread = std::shared_ptr<std::thread>();
	}
	//一些变量的重置
	//_ns_prebuf_pos = 0;
	_ns_after_pos = 0;
	_ns_after_read_pos = 0;
	_ns_cache_count = 0;
	_is_anc_start = false;
	_ns_level = 0;
}

void PortRecorder::addPaDevice(int deviceIdx, const PaDeviceInfo* dinfo)
{
	device_list[deviceIdx] = dinfo;
}