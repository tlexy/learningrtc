#include "peer_connection.h"
#include <qos/core/rtp_cacher.h>
#include <qos/entity/jetter_buffer_entity.h>
#include <chrono>
#include <endec/core/audio_io.h>
#include <audio/core/port_recorder.h>
#include <audio/common/audio_common.h>
#include <audio/core/audio_player.h>
#include <uvnet/utils/byte_order.hpp>
#include <common/util/file_saver.h>

#define SAVE_TEST

namespace tests
{
	PeerConnection::PeerConnection(std::shared_ptr<uvcore::UdpServer> server)
		:_udp_server(server)
	{
		_remote_addr.setPort(0);
#ifdef SAVE_TEST
		_aac_saver = new FileSaver(1024 * 1024*5, "test_aac", ".pcm");
#endif
	}

	void PeerConnection::set_recorder_device(int device_idx)
	{
		_audio_device_idx = device_idx;
	}

	void PeerConnection::remote_data_cb(uvcore::Udp*, const struct sockaddr* addr)
	{
		struct sockaddr_in* in_addr = (struct sockaddr_in*)addr;
		int port = sockets::networkToHost16(in_addr->sin_port);
		if (port != _remote_addr.getPort())
		{
			_remote_addr = uvcore::IpAddress::fromRawSocketAddress((struct sockaddr*)in_addr, sizeof(struct sockaddr_in));
		}
	}

	void PeerConnection::connect(const uvcore::IpAddress& ipaddr)
	{
		using namespace std::placeholders;

		if (_rtp_receiver)
		{
			return;
		}
		_sender_je = std::make_shared<AacJetterBufferEntity>();
		_sender_je->init();
		_sender_je->set_output_buffer(60);
		_remote_addr = ipaddr;
		_rtp_sender = std::make_shared<RtpSender>(_remote_addr, _sender_je, _udp_server);
		_rtp_sender->set_rtp_param(rtp_base::eAacLcPayLoad, 8765213);
		_rtp_sender->set_data_cb(std::bind(&PeerConnection::remote_data_cb, this, _1, _2));
		_rtp_sender->bind_local_addr();
	}

	void PeerConnection::listen(int local_port)
	{
		using namespace std::placeholders;

		if (_rtp_sender)
		{
			return;
		}
		_receiver_je = std::make_shared<AacJetterBufferEntity>();
		_receiver_je->init();
		_receiver_je->set_output_buffer(60);
		//uvcore::IpAddress local_addr(local_port);
		_local_addr.setPort(local_port);
		_rtp_receiver = std::make_shared<RtpReceiver>(_local_addr, _receiver_je, _udp_server);
		_rtp_receiver->set_data_cb(std::bind(&PeerConnection::remote_data_cb, this, _1, _2));
		_rtp_receiver->start();
		_receiver_th = std::make_shared<std::thread>(&PeerConnection::receiver_worker, this);
		_stop = false;
	}

	void PeerConnection::start_stream()
	{
		using namespace std::placeholders;

		//作为调用listen等待对端首先发送数据过来的一方，在收到数据之前，_rtp_sender一定是一个nullptr
		if (!_rtp_sender)
		{
			//不需要从对方接收RTP数据，所以jetter buffer参数传入nullptr
			_rtp_sender = std::make_shared<RtpSender>(_remote_addr, nullptr, _udp_server);
			_rtp_sender->set_rtp_param(rtp_base::eAacLcPayLoad, 8765215);
			_rtp_sender->bind_local_addr(_local_addr);
		}

		_audio_io = std::make_shared<AudioIO>(128000);
		_audio_io->set_io_cb(std::bind(&PeerConnection::recorder_enc_cb, this, _1, _2));
		_audio_io->start(_audio_device_idx);

		_sender_th = std::make_shared<std::thread>(&PeerConnection::sender_worker, this);
		_stop = false;
	}

	void PeerConnection::start_play()
	{
		using namespace std::placeholders;

		_audio_player = std::make_shared<AudioPlayer>();
		int ret = _audio_player->init_play();
		std::cout << "init playe ret: " << ret << std::endl;
		_audio_player->set_player_cb(std::bind(&PeerConnection::audio_player_cb, this, _1, _2));
		ret = _audio_player->play();
	}

	void PeerConnection::recorder_enc_cb(const uint8_t* data, int len)
	{
		_aac_timestamp += 1024;
		if (_rtp_sender)
		{
			_rtp_sender->send_rtp((void*)data, len, _aac_timestamp);
		}
//#ifdef SAVE_TEST
//		_aac_saver->write((const char*)data, len);
//#endif
	}

	void PeerConnection::audio_player_cb(void* output, unsigned long frameCount)
	{
		memset(output, 0x0, frameCount * 2 * 2);
		if (_receiver_je)
		{
			auto ptr = std::dynamic_pointer_cast<AacJetterBufferEntity>(_receiver_je);
			if (ptr)
			{
				int bytes = ptr->get_pcm_buffer((int8_t*)output, frameCount * 2 * 2);
				std::cout << "get bytes: " << bytes << "\t need：" << frameCount << std::endl;
#ifdef SAVE_TEST
				_aac_saver->write((const char*)output, bytes);
#endif
			}
			return;
		}
		if (_sender_je)
		{
			auto ptr = std::dynamic_pointer_cast<AacJetterBufferEntity>(_sender_je);
			if (ptr)
			{
				ptr->get_pcm_buffer((int8_t*)output, frameCount * 2 * 2);
			}
			return;
		}
	}

	void PeerConnection::receiver_worker()
	{
		while (!_stop)
		{
			if (_receiver_je)
			{
				_receiver_je->update();
				_receiver_je->decode();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void PeerConnection::sender_worker()
	{
		while (!_stop)
		{
			if (_sender_je)
			{
				_sender_je->update();
				_sender_je->decode();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void PeerConnection::destory()
	{
		_stop = true;
		if (_receiver_th && _receiver_th->joinable())
		{
			_receiver_th->join();
			_receiver_th = nullptr;
		}

		if (_sender_th && _sender_th->joinable())
		{
			_sender_th->join();
			_sender_th = nullptr;
		}
		if (_audio_io)
		{
			_audio_io->stop();
			_audio_io = nullptr;
		}
#ifdef SAVE_TEST
		_aac_saver->save();
#endif
	}
}