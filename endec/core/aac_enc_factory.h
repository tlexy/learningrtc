#ifndef AAC_SENDER_H
#define AAC_SENDER_H

#include <stdint.h>
#include <memory>
#include <thread>
#include <chrono>
#include <common/audio/loop_pcm_buffer.h>
#include <common/util/threadqueue.hpp>

class AacHelper;
class FileSaver;

class AacEncFactory
{
public:
	AacEncFactory(int numOfChannels = 2, int sampleRate = 44100, int bitRate = 192000);
	virtual ~AacEncFactory();

	bool isGood();
	//发送aac数据到内部线程进行编码，len的长度不能大于4096
	void sendFrame(const uint8_t*, int len);
	void startEncThread();
	void stopEncThread();
	//获取编码后的输出
	virtual void receivePacket(const uint8_t*, int len) = 0;

private:
	void enc_thread();

private:
	AacHelper* _aac_helper;
	bool _is_good;
	ThreadQueue<DcPcmBuffer*> _pcm_queue;
	DcLoopPcmBuffer* _pcm_bufque;
	std::shared_ptr<std::thread> _th;
	bool _is_stop{true};

	int _aac_temp_len{ 2048 };
	uint8_t* _aac_temp_buf;
};

class FileAacSender : public AacEncFactory
{
public:
	FileAacSender(int numOfChannels = 2, int sampleRate = 44100, int bitRate = 192000);

	virtual void receivePacket(const uint8_t*, int len);

private:
	FileSaver* _file;

};

#endif