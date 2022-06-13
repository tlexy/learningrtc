#include "aac_enc_factory.h"
#include "aac_helper.h"
#include <iostream>
#include <common/util/file_saver.h>
#include <time.h>
#include <string>

AacEncFactory::AacEncFactory(int numOfChannels, int sampleRate, int bitRate)
	:_pcm_queue(100),
	_th(std::shared_ptr<std::thread>())
{
	_aac_helper = new AacHelper();
	_is_good = _aac_helper->openEncoder(numOfChannels, sampleRate, bitRate);
	_pcm_bufque = new DcLoopPcmBuffer(43);
	_aac_temp_buf = (uint8_t*)malloc(_aac_temp_len);
}

AacEncFactory::~AacEncFactory()
{}

bool AacEncFactory::isGood()
{
	return _is_good;
}

void AacEncFactory::sendFrame(const uint8_t* data, int len)
{
	if (len > 4096)
	{
		std::cout << "sendFrame 0, data too long: " << len << std::endl;
	}
	if (!isGood())
	{
		std::cout << "sendFrame 1" << std::endl;
		return;
	}
	std::cout << "receive PCM, LEN: " << len << std::endl;
	DcPcmBuffer* ptr = _pcm_bufque->get_empty_buffer2();
	if (ptr == NULL)
	{
		std::cout << "sendFrame 2" << std::endl;
		return;
	}
	memcpy(ptr->buffer[0], data, len);
	ptr->buff_size = len;
	_pcm_queue.push_back(ptr);
}

void AacEncFactory::enc_thread()
{
	_is_stop = false;
	bool flag = false;
	int ret = 0;
	while (!_is_stop)
	{
		DcPcmBuffer* ptr = _pcm_queue.pop(flag, std::chrono::milliseconds(10));
		if (flag && ptr != NULL)
		{
			_aac_temp_len = 2048;
			ret = _aac_helper->encode(ptr->buffer[0], ptr->buff_size, _aac_temp_buf, _aac_temp_len);
			if (ret == 0)
			{
				if (_aac_temp_len > 0)
				{
					receivePacket(_aac_temp_buf, _aac_temp_len);
				}
			}
			else
			{
				std::cout << "aac encode error: " << ret << "\tlen: " << _aac_temp_len << std::endl;
			}
		}
		else
		{
			/*if (flag == false)
			{
				std::cout << "pcm queue is empty..." << std::endl;
			}
			if (ptr == NULL)
			{
				std::cout << "ptr is NULL." << std::endl;
			}*/
		}
		if (ptr != NULL)
		{
			_pcm_bufque->push_empty_buffer2(ptr);
		}
	}
}

void AacEncFactory::startEncThread()
{
	if (_th)
	{
		return;
	}
	_th = std::make_shared<std::thread>(&AacEncFactory::enc_thread, this);
}

void AacEncFactory::stopEncThread()
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
	:AacEncFactory(numOfChannels, sampleRate, bitRate)
{
	_file = new FileSaver(1024 * 1024, "demo", ".aac");
}

void FileAacSender::receivePacket(const uint8_t* buf, int len)
{
	std::cout << "receivePacket, len: " << len << std::endl;
	_file->write((const char*)buf, len);
}