#include "audio_player.h"
#include <stdlib.h>

#define SAMPLE_RATE         (44100)
#define FRAMES_PER_BUFFER   (1024)

#pragma execution_character_set("utf-8")

std::unordered_map<int, const PaDeviceInfo*> AudioPlayer::_device_list;

AudioPlayer::AudioPlayer()
	:_stream(NULL),
	_sample_rate(SAMPLE_RATE)
{
}

void AudioPlayer::add_pa_device(int deviceIdx, const PaDeviceInfo* dinfo)
{
	_device_list[deviceIdx] = dinfo;
}

void AudioPlayer::set_sample_rate(int sample_rate)
{
	_sample_rate = sample_rate;
}

uint32_t AudioPlayer::format()
{
	return _format;
}

int AudioPlayer::sample_rate()
{
	return _sample_rate;
}

int play_cb(const void* input, void* output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	AudioPlayer* player = (AudioPlayer*)userData;
	player->read_data(output, frameCount);
	return paContinue;
}

void AudioPlayer::read_data(void* dst, int frameCount)
{
	if (_is_stop)
	{
		return;
	}
	int size = frameCount * _channel_count * _deps;
	memset(dst, 0x0, size);
	if (_cb)
	{
		_cb(dst, frameCount);
	}
	else
	{
		std::lock_guard<std::mutex> lock(_play_buff_mutex);
		int count = _play_buff->read((uint8_t*)dst, size);
	}
}

void AudioPlayer::fill_data(const uint8_t* data, int len)
{
	std::lock_guard<std::mutex> lock(_play_buff_mutex);
	_play_buff->push(data, len);
}

void AudioPlayer::set_player_cb(AudioPlayerCallBack cb)
{
	_cb = cb;
}

int AudioPlayer::play()
{
	_is_stop = false;
	//return play(Pa_GetDefaultOutputDevice());
	int err = Pa_StartStream(_stream);
	if (err != paNoError)
	{
		return -3;
	}
	return 0;
}

void AudioPlayer::stop()
{
	pause();
	if (_stream)
	{
		Pa_AbortStream(_stream);
		Pa_StopStream(_stream);
		_stream = NULL;
	}
}

void AudioPlayer::pause()
{
	_is_stop = true;
}

int AudioPlayer::channel_count()
{
	return _channel_count;
}

int AudioPlayer::init_play()
{
	return init_play(Pa_GetDefaultOutputDevice());
}

int AudioPlayer::init_play(int device_idx)
{
	if (_stream)
	{
		Pa_AbortStream(_stream);
		Pa_StopStream(_stream);
		_stream = NULL;
	}
	//默认参数
	
	_format = paInt16;
	_deps = 2;

	_play_buff = new AudioBuffer(_sample_rate* _deps* _channel_count *30);//双通道、16位、44100下，最长缓冲30s的长度
	const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(device_idx);
	if (!dinfo)
	{
		return -1;
	}

	PaStream* out_stream;
	PaStreamParameters out_params;
	out_params.channelCount = 2;//固定双通道
	out_params.device = device_idx;
	out_params.sampleFormat = paInt16;
	out_params.hostApiSpecificStreamInfo = NULL;
	out_params.suggestedLatency = dinfo->defaultHighInputLatency;

	PaError err = Pa_IsFormatSupported(NULL, &out_params, SAMPLE_RATE);
	if (err != paNoError)
	{
		_sample_rate = dinfo->defaultSampleRate;
		_format = paFloat32;
		_deps = 4;
		out_params.sampleFormat = paFloat32;
		err = Pa_IsFormatSupported(NULL, &out_params, dinfo->defaultSampleRate);
		if (err != paNoError)
		{
			return -1;
		}
	}
	err = Pa_OpenStream(
		&out_stream,
		NULL, /* no input */
		&out_params,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff,      /* we won't output out of range samples so don't bother clipping them */
		play_cb, /* no callback, use blocking API */
		this); /* no callback, so no callback userData */
	if (err != paNoError)
	{
		return -2;
	}
	_stream = out_stream;
	return 0;
}