#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include <functional>
#include <stdint.h>
#include "aac_enc_factory.h"
#include <audio/core/port_recorder.h>
#include <audio/core/iport_callback.h>

using AudioIoCallBack = std::function<void(const uint8_t*, int len)>;

class PortRecorder;

class AudioIO : public AacEncFactory, public IPortCallBack
{
public:
	AudioIO(int bitRate);
	//PCM采集回调
	virtual void stream_cb(const void* input, unsigned long frameCount, int sampleSize);
	//设置AAC编码后的回调
	void set_io_cb(AudioIoCallBack);

	void start(int pa_device_index = -1);
	void stop();

protected:
	//收到编码后的（一帧）数据
	virtual void receivePacket(const uint8_t*, int len);

private:
	AudioIoCallBack _cb{};
	PortRecorder* _recorder{};
	int _sample_rate{ 44100 };
	int _sample_deps{ 2 };
	int _sample_channel{ 2 };

	FileSaver* _file_saver{nullptr};
};

#endif