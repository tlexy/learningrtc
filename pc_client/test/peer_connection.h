#ifndef LEARNING_RTC_PEER_CONNECTION_H
#define LEARNING_RTC_PEER_CONNECTION_H

#include <memory>
#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <thread>
#include <qos/core/rtp_receiver.h>
#include <qos/core/rtp_sender.h>

#include <video/test/x264_encoder.h>

class JitterBufferEntity;
class StreamsJitterBufferEntity;
//class RtpReceiver;
//class RtpSender;
class AudioIO;
class AudioPlayer;
class FileSaver;

class RtpH264Encoder;
class RtpH264Decoder;

namespace uvcore
{
	class Udp;
}

namespace webrtc::test
{
	class  VcmCapturer;
}

namespace tests
{
	class PeerConnection
	{
	public:
		PeerConnection(std::shared_ptr<uvcore::UdpServer>);
		void set_recorder_device(int device_idx);
		void connect(const uvcore::IpAddress& ipaddr);

		/// <summary>
		/// 开始本地录制以及推流到对端
		/// </summary>
		void start_stream(int width, int height);

		void set_video_capturer(webrtc::test::VcmCapturer*);

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

		void h264_enc_cb(uint8_t*, int);

	private:
		std::shared_ptr<uvcore::UdpServer> _udp_server;
		std::shared_ptr<RtpReceiver> _rtp_receiver{nullptr};
		std::shared_ptr<StreamsJitterBufferEntity> _receiver_je;
		uvcore::IpAddress _remote_addr;
		uvcore::IpAddress _local_addr;//接收端的本地接收端口
		bool _stop{ true };
		//接收端接收对端发过来的RTP包的处理线程
		std::shared_ptr<std::thread> _receiver_th{nullptr};
		//发送端接收对端RTP包的返回处理线程
		std::shared_ptr<std::thread> _sender_th{ nullptr };
		std::shared_ptr<RtpSender> _rtp_sender{nullptr};
		std::shared_ptr<StreamsJitterBufferEntity> _sender_je;
		std::shared_ptr<AudioIO> _audio_io;
		uint32_t _aac_timestamp{0};
		int _audio_device_idx;
		std::shared_ptr<AudioPlayer> _audio_player;

		int _video_width;
		int _video_height;
		std::shared_ptr<X264Encoder> _x264_encoder;
		std::shared_ptr<RtpH264Encoder> _rtp_h264_encoder{nullptr};
		std::shared_ptr<RtpH264Decoder> _rtp_h264_decoder;
		webrtc::test::VcmCapturer* _vcm_capturer{nullptr};
		rtp_base::rtc_ext_header _rtc_v_hdr;

		///for test
		FileSaver* _aac_saver{nullptr};
	};
}

#endif