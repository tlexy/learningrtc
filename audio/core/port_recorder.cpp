#include "port_recorder.h"
#include "iport_callback.h"
#include "core/common_log.h"
#include <3rd/log4u/core/common_log.h>

#pragma execution_character_set("utf-8")

#define PORT_AAC_FRAME 4096
#define PORT_SAMPLES_PER_FRAME 1024
#define PORT_INPUT_TEMP (PORT_SAMPLES_PER_FRAME * 2 * 4)
//#define TEST_SAVE_DUMP

std::map<int, const PaDeviceInfo*> PortRecorder::device_list;

PortRecorder::PortRecorder(IPortCallBack* cb)
	:cb(cb),
	_record_thread(std::shared_ptr<std::thread>()),
	swr_ctx(NULL),
	is_record_stop(true)
{
	record_mid = new mid_buf(1024 * 1024);

	//_target_swr_ctx = init_swr(32000, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
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

bool PortRecorder::start_record(int paDeviceIndex)
{
	if (paDeviceIndex < 0)
	{
		//paDeviceIndex = Pa_GetDefaultInputDevice();
		if (device_list.size() > 0)
		{
			paDeviceIndex = device_list.begin()->first;
		}
	}
	if (device_list.find(paDeviceIndex) == device_list.end())
	{
		return false;
	}
	/*src_rate = device_list[paDeviceIndex]->defaultSampleRate;
	if (src_rate < 44100)
	{
		return false;
	}*/
	if (_record_thread)
	{
		return false;
	}
	//当前采样是否支持？
	swr_ctx = NULL;

	is_record_stop = false;
	_record_thread = std::make_shared<std::thread>(&PortRecorder::thread_record, this, paDeviceIndex);
}

static char port_input_temp_buff[PORT_INPUT_TEMP];//最大float32
static char port_swr_buff[PORT_INPUT_TEMP*2];//最大float32

static int64_t frame_time = 0;

int port_record_cb(
	const void* input, void* output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	PortRecorder* recorder = (PortRecorder*)userData;
	if (recorder->is_record_stop)
	{
		return paComplete;
	}
	int framesize = frameCount * recorder->sample_size * recorder->channel_size;
	if (framesize < 1)
	{
		log_error("framesize too small");
	}
	recorder->record_mid->push((const uint8_t*)input, framesize);
	if (!recorder->cb)
	{
		return paContinue;
	}
	while (recorder->record_mid->usable_count() >= recorder->tem_mill_size)
	{
		if (recorder->swr_ctx != NULL)
		{
			int count = recorder->do_swr(recorder->swr_ctx, recorder->record_mid->data(), recorder->ten_mill_sample, recorder->src_rate,
				port_swr_buff, sizeof(port_swr_buff), recorder->dst_rate);

			recorder->cb->stream_cb(port_swr_buff, count, 16);
		}
		else
		{
			//不需要重采样，直接复制
			recorder->cb->stream_cb(recorder->record_mid->data(), frameCount, 16);
		}
		recorder->record_mid->set_read(PORT_AAC_FRAME);
	}

	return paContinue;
}

void PortRecorder::set_target_rate(int sample_rate)
{
	if (sample_rate == 32000
		|| sample_rate == 44100
		|| sample_rate == 48000)
	{
		dst_rate = sample_rate;
	}
}

void PortRecorder::thread_record(int index)
{
	const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(index);
	if (!dinfo)
	{
		log_error("Pa_GetDeviceInfo return NULL");
		return;
	}

	PaStream* inputStream;
	PaStreamParameters inputParam;
	inputParam.channelCount = 2;
	inputParam.device = index;
	inputParam.sampleFormat = paInt16;// paFloat32;

	inputParam.hostApiSpecificStreamInfo = NULL;
	inputParam.suggestedLatency = dinfo->defaultLowInputLatency;//dinfo->defaultLowInputLatency;
	/*if (inputParam.sampleFormat == paFloat32)
	{
		sample_size = 4;
	}
	else
	{
		sample_size = 2;
	}*/
	src_rate = dst_rate;
	PaError err = Pa_IsFormatSupported(&inputParam, NULL, src_rate);
	if (err != paNoError)
	{
		//sample_size = 2;
		/*log_e("port recorder, format not support, index: %d, name: %s, sampleFormat: %d, sampleRate: %d", index, dinfo->name, inputParam.sampleFormat, dinfo->defaultSampleRate);
		log_e("port recorder, try 16bit");*/
		inputParam.sampleFormat = paInt16;
		src_rate = dinfo->defaultSampleRate;
		err = Pa_IsFormatSupported(&inputParam, NULL, src_rate);
		if (err != paNoError)
		{
			log_error("audio format unsupported");
			return;
		}
	}
	
	if (inputParam.sampleFormat == paInt16 && src_rate == dst_rate)
	{
		swr_ctx = NULL;
	}
	else
	{
		AVSampleFormat src_fmt = AV_SAMPLE_FMT_FLT;
		if (inputParam.sampleFormat == paInt16)
		{
			src_fmt = AV_SAMPLE_FMT_S16;
		}
		src_fmt = src_fmt;
		ten_mill_sample = dst_rate / 100;
		tem_mill_size = ten_mill_sample * sample_size * channel_size;
		swr_ctx = init_swr(src_rate, AV_CH_LAYOUT_STEREO, src_fmt, dst_rate, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
	}
	//log_w("port recorder open stream: index: %d, name: %s, sampleFormat: %d, sampleRate: %f", index, dinfo->name, inputParam.sampleFormat, dinfo->defaultSampleRate);
	err = Pa_OpenStream(&inputStream, &inputParam, NULL, src_rate, paFramesPerBufferUnspecified, 0, port_record_cb, this);
	
	err = Pa_StartStream(inputStream);
	if (err != paNoError)
	{
		log_error("pa start stream error");
		return;
	}
	_stream = inputStream;
}

void PortRecorder::stop_record()
{
	log_error("stopRecord...");
	is_record_stop = true;
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

void PortRecorder::add_pa_device(int deviceIdx, const PaDeviceInfo* dinfo)
{
	device_list[deviceIdx] = dinfo;
}