#include "peer_connection.h"
#include <qos/core/rtp_cacher.h>
#include <qos/core/rtp_receiver.h>
#include <qos/core/rtp_sender.h>
#include <qos/entity/jetter_buffer_entity.h>
#include <chrono>
#include <endec/core/audio_io.h>
#include <audio/core/port_recorder.h>
#include <audio/common/audio_common.h>
#include <audio/core/audio_player.h>

namespace tests
{
	PeerConnection::PeerConnection()
	{
	}

	void PeerConnection::set_audio_device(int device_idx)
	{
		_audio_device_idx = device_idx;
	}

	void PeerConnection::connect(const uvcore::IpAddress& ipaddr)
	{
		_sender_je = std::make_shared<AacJetterBufferEntity>();
		_sender_je->init();
		_sender_je->set_output_buffer(60);
		_remote_addr = ipaddr;
		_rtp_sender = std::make_shared<RtpSender>(_remote_addr, _sender_je, _udp_server);
		_rtp_sender->set_rtp_param(rtp_base::eAacLcPayLoad, 8765213);
	}

	void PeerConnection::start_stream()
	{
		using namespace std::placeholders;

		_audio_io = std::make_shared<AudioIO>(44100);
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
	}

	void PeerConnection::audio_player_cb(void* output, unsigned long frameCount)
	{
		//
	}

	void PeerConnection::listen(int local_port)
	{
		_receiver_je = std::make_shared<AacJetterBufferEntity>();
		_receiver_je->init();
		_receiver_je->set_output_buffer(60);
		uvcore::IpAddress local_addr(local_port);
		_rtp_receiver = std::make_shared<RtpReceiver>(local_addr, _receiver_je, _udp_server);

		_receiver_th = std::make_shared<std::thread>(&PeerConnection::receiver_worker, this);
		_stop = false;
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
	}
}