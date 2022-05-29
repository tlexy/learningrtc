#ifndef LEARNING_RTC_PEER_CONNECTION_H
#define LEARNING_RTC_PEER_CONNECTION_H

#include <memory>
#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <thread>
#include <qos/core/rtp_receiver.h>
#include <qos/core/rtp_sender.h>

class JetterBufferEntity;
//class RtpReceiver;
//class RtpSender;
class AudioIO;
class AudioPlayer;

namespace uvcore
{
	class Udp;
}

namespace tests
{
	class PeerConnection
	{
	public:
		PeerConnection(std::shared_ptr<uvcore::UdpServer>);
		void set_audio_device(int device_idx);
		void connect(const uvcore::IpAddress& ipaddr);

		/// <summary>
		/// 开始本地录制以及推流到对端
		/// </summary>
		void start_stream();

		/// <summary>
		/// 开始播放接收到的音频
		/// </summary>
		void start_play();

		void listen(int local_port);

		void destory(); 

	private:
		void receiver_worker();
		void sender_worker();

		void recorder_enc_cb(const uint8_t*, int len);
		void audio_player_cb(void* output, unsigned long frameCount);

		void remote_data_cb(uvcore::Udp*, const struct sockaddr*);

	private:
		std::shared_ptr<uvcore::UdpServer> _udp_server;
		std::shared_ptr<RtpReceiver> _rtp_receiver{nullptr};
		std::shared_ptr<JetterBufferEntity> _receiver_je;
		uvcore::IpAddress _remote_addr;
		bool _stop{ true };
		//接收端接收对端发过来的RTP包的处理线程
		std::shared_ptr<std::thread> _receiver_th{nullptr};
		//发送端接收对端RTP包的返回处理线程
		std::shared_ptr<std::thread> _sender_th{ nullptr };
		std::shared_ptr<RtpSender> _rtp_sender{nullptr};
		std::shared_ptr<JetterBufferEntity> _sender_je;
		std::shared_ptr<AudioIO> _audio_io;
		uint32_t _aac_timestamp{0};
		int _audio_device_idx;
		std::shared_ptr<AudioPlayer> _audio_player;
	};
}

#endif