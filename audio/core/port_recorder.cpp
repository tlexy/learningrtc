#include "port_recorder.h"
#include "iport_callback.h"
#include <stdio.h>
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
	_sample_size(0)
{
	_recorder_id = _recorder_id_s++;

}

PortRecorder::~PortRecorder()
{
	
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
	printf("start record, idx: %d\n", paDeviceIndex);
	_src_rate = 0;
	if (device_list.find(paDeviceIndex) == device_list.end())
	{
		printf("port recorder, device index not exist: %d\n", paDeviceIndex);
		return false;
	}
	_src_rate = device_list[paDeviceIndex]->defaultSampleRate;
	printf("start record, idx: %d, defaultSampleRate: %d\n", paDeviceIndex, _src_rate);
	if (_src_rate < 44100)
	{
		printf("port recorder, sample rate too small: %d\n", _src_rate);
		return false;
	}
	if (_record_thread)
	{
		printf("port recorder, thread is running\n");
		return false;
	}
	//当前采样是否支持？
	_swr_ctx = NULL;

	_frame_size = 1024;
	if (_src_rate == 48000)
	{
		_frame_size = 1115;
	}
	_is_record_stop = false;
	_record_thread = std::make_shared<std::thread>(&PortRecorder::thread_record, this, paDeviceIndex);
	return true;
}

void PortRecorder::set_target_sample_rate(int sample_rate)
{
	_target_sample_rate = sample_rate;
}


static char port_input_temp_buff[PORT_INPUT_TEMP];//最大float32
static char port_swr_buff[PORT_INPUT_TEMP*2];//最大float32

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
	err = Pa_StartStream(inputStream);
	if (err != paNoError)
	{
		log_e("start error: ", Pa_GetErrorText(err));
	}
	_stream = inputStream;
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
}

void PortRecorder::addPaDevice(int deviceIdx, const PaDeviceInfo* dinfo)
{
	device_list[deviceIdx] = dinfo;
}