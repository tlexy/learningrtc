#include "peer_connection.h"
#include <qos/core/rtp_cacher.h>
#include <qos/entity/jitter_buffer_entity.h>
#include <qos/entity/streams_jitter_buffer_entity.h>
#include <chrono>
#include <endec/core/audio_io.h>
#include <audio/core/port_recorder.h>
#include <audio/common/audio_common.h>
#include <audio/core/audio_player.h>
#include <uvnet/utils/byte_order.hpp>
#include <common/util/file_saver.h>

#include <video/rtp_h264/rtp_h264_encoder.h>
#include <video/rtp_h264/rtp_h264_decoder.h>
#include <webrtc_camera/core/video_capture.h>
#include <webrtc_camera/core/video_capture_factory.h>
#include <webrtc_camera/vcm_capturer.h>
#include <QDebug>
#include "../common/pc_global.h"
#include "../component/comm_thread.h"

#define SAVE_TEST

namespace tests
{
	PeerConnection::PeerConnection(std::shared_ptr<uvcore::UdpServer> server)
		:_udp_server(server)
	{
		_remote_addr.setPort(0);
		_rtc_v_hdr.ext_seq = 0;
#ifdef SAVE_TEST
		_aac_saver = new FileSaver(1024 * 1024*5, "test_aac", ".pcm");
#endif
	}

	void PeerConnection::set_recorder_device(int device_idx)
	{
		_audio_device_idx = device_idx;
	}

	void PeerConnection::set_video_capturer(webrtc::test::VcmCapturer* capturer)
	{
		_vcm_capturer = capturer;
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
		_sender_je = std::make_shared<StreamsJitterBufferEntity>();
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
		//_receiver_je = std::make_shared<AacJitterBufferEntity>();
		_receiver_je = std::make_shared< StreamsJitterBufferEntity>();
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

	void PeerConnection::start_stream(int width, int height)
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
		//音频相关
		_audio_io = std::make_shared<AudioIO>(128000);
		_audio_io->set_io_cb(std::bind(&PeerConnection::recorder_enc_cb, this, _1, _2));
		_audio_io->start(_audio_device_idx);
		//视频相关
		_video_width = width;
		_video_height = height;
		{
			VideoParameter vp;
			vp.width = width;
			vp.height = height;
			vp.fps = 30;
			SignalHub sig;
			sig.first = eSigVideoReady;
			sig.t = std::make_any<VideoParameter>(vp);
			PcGlobal::get_instance()->comm_thread()->push(sig);
		}
		_x264_encoder = std::make_shared<X264Encoder>();
		_x264_encoder->init(_video_width, _video_height, 30);
		_x264_encoder->set_enc_cb(std::bind(&PeerConnection::h264_enc_cb, this, _1, _2));
		if (_vcm_capturer)
		{
			_vcm_capturer->AddSubscriber(_x264_encoder);
		}
		_x264_encoder->start();

		if (_vcm_capturer)
		{
			//_vcm_capturer->StartCapture();
		}
		else
		{
			qDebug() << "vcm capture is nullptr";
		}
		//音频接收？
		_sender_th = std::make_shared<std::thread>(&PeerConnection::sender_worker, this);
		_stop = false;
	}

	void PeerConnection::start_play()
	{
		using namespace std::placeholders;

		_audio_player = std::make_shared<AudioPlayer>();
		int ret = _audio_player->init_play();
		std::cout << "init player ret: " << ret << std::endl;
		_audio_player->set_player_cb(std::bind(&PeerConnection::audio_player_cb, this, _1, _2));
		ret = _audio_player->play();
	}

	void PeerConnection::h264_enc_cb(uint8_t* payload, int payload_len)
	{
		//return;
		//封装为rtp格式（载荷类型为h264）并发送出去
		if (!_rtp_h264_encoder)
		{
			_rtp_h264_encoder = std::make_shared<RtpH264Encoder>();
			rtp_parameter param;
			memset(&param, 0x0, sizeof(param));
			param.pt = rtp_base::eH264PayLoad;
			param.version = 2;

			rtp_session sess;
			sess.seq_number = 10086;
			sess.timestamp = 0;
			sess.ssrc = 80136561l;
			_rtp_h264_encoder->init(param, sess, 3000);
		}
		int off = 3;
		unsigned char* p = payload;
		if (p[0] == 0x00 && p[1] == 0x00
			&& p[2] == 0x00 && p[3] == 0x01)
		{
			off = 4;
		}
		qDebug() << "h264 data comming, len: " << payload_len;
		_rtp_h264_encoder->encode((const char*)payload + off, payload_len - off);
		rtp_packet_t* rtp;
		int rr = _rtp_h264_encoder->get_packet(rtp);
		while (rr >= 0)
		{
			rtp_copy_ext_hdr(rtp, rtp_base::eGroupExt, sizeof(rtp_base::rtc_ext_header), &_rtc_v_hdr);
			_rtp_sender->send_rtp_packet(rtp);
			rr = _rtp_h264_encoder->get_packet(rtp);
		}
	}

	void PeerConnection::recorder_enc_cb(const uint8_t* data, int len)
	{
		_aac_timestamp += 1024;
		if (_rtp_sender)
		{
			_rtp_sender->send_raw_data((void*)data, len, _aac_timestamp);
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
			/*auto ptr = std::dynamic_pointer_cast<StreamsJitterBufferEntity>(_receiver_je);
			if (ptr)
			{*/
				int64_t pts = 0;
				int bytes = _receiver_je->get_pcm_buffer((int8_t*)output, frameCount * 2 * 2, pts);
				std::cout << "get bytes: " << bytes << "\t need：" << frameCount << std::endl;
#ifdef SAVE_TEST
				_aac_saver->write((const char*)output, bytes);
#endif
			//}
			return;
		}
		if (_sender_je)
		{
			auto ptr = std::dynamic_pointer_cast<AacJitterBufferEntity>(_sender_je);
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
				//尝试获得视频及其参数，通过信号通知UI界面
				AVFrame* av_frame = nullptr;
				int ret = _receiver_je->get_video_frame_front(av_frame);
				if (ret == 1 && av_frame)
				{
					//_vcm_capturer->PushFrame(av_frame);
					VideoFrame vf;
					vf.frame = av_frame;
					PUSH_SIG(eSigVideoFrame, vf, VideoFrame);
					if (!_video_ready)
					{
						VideoParameter vp;
						_receiver_je->get_video_info(vp.width, vp.height, vp.fps);
						SignalHub sig;
						sig.first = eSigVideoReady;
						sig.t = std::make_any<VideoParameter>(vp);
						PcGlobal::get_instance()->comm_thread()->push(sig);
						_video_ready = true;
					}
				}
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
		/*if (_vcm_capturer)
		{
			delete _vcm_capturer;
			_vcm_capturer = nullptr;
		}*/
		if (_receiver_je)
		{
			_receiver_je->destory();
		}
		if (_x264_encoder)
		{
			_x264_encoder->stop();
			_x264_encoder = nullptr;
		}
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