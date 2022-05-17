#include "audio_io.h"
#include <audio/core/port_recorder.h>
#include <audio/common/audio_common.h>

AudioIO::AudioIO(int bitRate)
	:AacEncFactory(2, 44100, bitRate)
{
	_recorder = new PortRecorder(this);
}

void AudioIO::set_io_cb(AudioIoCallBack cb)
{
	_cb = cb;
}

void AudioIO::start(int pa_device_index)
{
	_recorder->set_target_rate(_sample_rate);
	_recorder->start_record(pa_device_index);

	startEncThread();
}

void AudioIO::stop()
{
	stopEncThread();
	_recorder->stop_record();
}

void AudioIO::receivePacket(const uint8_t* data, int len)
{
	if (_cb)
	{
		_cb(data, len);
	}
}

void AudioIO::stream_cb(const void* input, unsigned long frameCount, int sampleSize)
{
	int framesize = frameCount * sampleSize * _sample_channel/8;
	sendFrame((uint8_t*)input, framesize);
}