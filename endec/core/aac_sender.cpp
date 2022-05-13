#include "aac_sender.h"
#include "aac_helper.h"
#include <iostream>
#include <common/util/file_saver.h>

AacSender::AacSender(int numOfChannels, int sampleRate, int bitRate)
	:_pcm_queue(100),
	_th(std::shared_ptr<std::thread>())
{
	_aac_helper = new AacHelper();
	_is_good = _aac_helper->openEncoder(numOfChannels, sampleRate, bitRate);
	_pcm_bufque = new DcLoopPcmBuffer(43, 1);
	_aac_temp_buf = (uint8_t*)malloc(_aac_temp_len);
}

AacSender::~AacSender()
{}

bool AacSender::isGood()
{
	return _is_good;
}

void AacSender::sendFrame(const uint8_t* data, int len)
{
	//std::cout << "sendFrame, len:" << len << std::endl;
	if (!isGood())
	{
		std::cout << "sendFrame 1" << std::endl;
		return;
	}
	DcPcmBuffer* ptr = _pcm_bufque->get_empty_buffer();
	if (ptr == NULL)
	{
		std::cout << "sendFrame 2" << std::endl;
		return;
	}
	memcpy(ptr->buffer[0], data, len);
	_pcm_queue.push_back(ptr);
}

void AacSender::enc_thread()
{
	_is_stop = false;
	bool flag = false;
	int ret = 0;
	while (!_is_stop)
	{
		DcPcmBuffer* ptr = _pcm_queue.pop(flag, std::chrono::milliseconds(10000));
		if (flag && ptr != NULL)
		{
			_aac_temp_len = 2048;
			ret = _aac_helper->encode(ptr->buffer[0], ptr->buff_size, _aac_temp_buf, _aac_temp_len);
			if (ret == 0 && _aac_temp_len > 0)
			{
				receivePacket(_aac_temp_buf, _aac_temp_len);
			}
			else
			{
				std::cout << "aac encode error: " << ret << "\tlen: " << _aac_temp_len << std::endl;
			}
		}
		else
		{
			if (flag == false)
			{
				std::cout << "pcm queue is empty..." << std::endl;
			}
			if (ptr == NULL)
			{
				std::cout << "ptr is NULL." << std::endl;
			}
		}
		if (ptr != NULL)
		{
			_pcm_bufque->push_empty_buffer(ptr);
		}
	}
}

void AacSender::startEncThread()
{
	if (_th)
	{
		return;
	}
	_th = std::make_shared<std::thread>(&AacSender::enc_thread, this);
}

void AacSender::stopEncThread()
{
	_is_stop = true;
	if (_th)
	{
		_th->join();
		_th = std::shared_ptr<std::thread>();
	}
}


///////////////////////////////////


FileAacSender::FileAacSender(int numOfChannels, int sampleRate, int bitRate)
	:AacSender(numOfChannels, sampleRate, bitRate)
{
	_file = new FileSaver(1024 * 1024, "demo");
}

void FileAacSender::receivePacket(const uint8_t* buf, int len)
{
	std::cout << "receivePacket, len: " << len << std::endl;
	_file->write((const char*)buf, len);
}