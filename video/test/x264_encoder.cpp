#include "x264_encoder.h"
#include <webrtc_camera/video_frame/video_frame_buffer.h>
#include <webrtc_camera/video_frame/i420_buffer.h>
#include "file_saver.h"
#include <iostream>
#include <assert.h>
#include <3rd/uvnet/utils/sock_utils.h>
#include <rtp_base/core/rtp_base_common_def.h>
#include <3rd/uvnet/utils/byte_order.hpp>

X264Encoder::X264Encoder()
	:webrtc::test::VideoFrameSubscriber()
{
#ifdef SAVEF
	_h264_file = new FileSaver(1024*1024*100, "h264_i420_", ".h264");
	_rtp_save = new FileSaver(1024 * 1024 * 100, "rtp_i420_", ".h264");
#endif
	_rhd = new RtpH264Decoder;
}

void X264Encoder::OnFrame(const webrtc::VideoFrame& frame)
{
	_qu.push_back(frame);
}

void X264Encoder::init(int width, int height, int fps)
{
	_width = width;
	_height = height;
	_fps = fps;

	/*x264_param_t mParam;
	x264_param_default(&mParam);*/

	int iNal = 0;
	x264_nal_t* pNals = NULL;
	x264_t* pHandle = NULL;
	x264_param_t* pParam = (x264_param_t*)malloc(sizeof(x264_param_t));
	x264_param_default(pParam);   //�������ṹ�帳Ĭ��ֵ
	//����preset��tune
	x264_param_default_preset(pParam, "fast", "zerolatency");
	pParam->i_csp = X264_CSP_I420;//yuv420p
	pParam->i_width = width;   // ���
	pParam->i_height = height;  // �߶�
	pParam->i_fps_num = 30;     // ����֡�ʣ����ӣ�
	pParam->i_fps_den = 1;      // ����֡��ʱ��1s����ĸ��

	pParam->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
	pParam->i_keyint_max = 30;              //�ڴ˼������IDR�ؼ�֡
	///slice :live ֱ��
	pParam->i_slice_count = 1;
	pParam->i_timebase_den = 30;
	pParam->i_timebase_num = 1;

	pParam->rc.i_bitrate = 500;       // ��������,��ABR(ƽ������)ģʽ�²���Ч���ұ���������ABRǰ������bitrate
	pParam->rc.i_rc_method = X264_RC_ABR;  // ���ʿ��Ʒ�����CQP(�㶨����)��CRF(�㶨����,ȱʡֵ23)��ABR(ƽ������)
	x264_param_apply_profile(pParam, "high");//baseline

	//open encoder
	pHandle = x264_encoder_open(pParam);
	_handle = pHandle;
	_param = pParam;
}

void X264Encoder::start()
{
	_is_stop = false;
	if (!_th)
	{
		_th = std::make_shared<std::thread>(&X264Encoder::encode_thread, this);
	}
}

void X264Encoder::stop()
{
	_is_stop = true;
	if (_th)
	{
		_th->join();
		_th = nullptr;
	}
#ifdef SAVEF
	_h264_file->save();
	_rtp_save->save();
#endif
}

void X264Encoder::set_enc_cb(X264VideoEncCb cb)
{
	_cb = cb;
}

void X264Encoder::encode_thread()
{
	x264_picture_t* pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_picture_t* pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_picture_init(pPic_out);//out.picture
	//I420,yuv420p
	x264_picture_alloc(pPic_in, X264_CSP_I420, _param->i_width, _param->i_height);
	bool flag = false;
	int iNal = 0;
	x264_nal_t* pNals = NULL;
	int ret = 0;
	int counter = 0;

	int sockfd = sockets::Socket(AF_INET, SOCK_DGRAM, 0);
	const char* ipstr = "127.0.0.1";
	RtpH264Encoder* rh = new RtpH264Encoder;
	rtp_parameter param;
	memset(&param, 0x0, sizeof(param));
	param.pt = rtp_base::eH264PayLoad;
	param.version = 2;

	rtp_session sess;
	sess.seq_number = 10086;
	sess.timestamp = 0;
	sess.ssrc = 80136561l;
	rh->init(param, sess, 3000);

	while (!_is_stop) 
	{
		webrtc::VideoFrame frame = _qu.pop(flag, std::chrono::milliseconds(100));
		if (flag)
		{
			assert(_param->i_height == frame.video_frame_buffer()->GetI420()->height()
				&& _param->i_width == frame.video_frame_buffer()->GetI420()->width());
			/*_h264_file->write((const char*)frame.video_frame_buffer()->GetI420()->DataY(), frame.video_frame_buffer()->GetI420()->StrideY());
			_h264_file->write((const char*)frame.video_frame_buffer()->GetI420()->DataU(), frame.video_frame_buffer()->GetI420()->StrideU());
			_h264_file->write((const char*)frame.video_frame_buffer()->GetI420()->DataV(), frame.video_frame_buffer()->GetI420()->StrideV());*/
			/*std::cout << "stride: " << frame.video_frame_buffer()->GetI420()->StrideY() << ":" << frame.video_frame_buffer()->GetI420()->StrideU()
				<< ":" << frame.video_frame_buffer()->GetI420()->StrideV() << "|" << frame.video_frame_buffer()->GetI420()->height() << ":"
				<< frame.video_frame_buffer()->GetI420()->width() << std::endl;*/
			int y = frame.video_frame_buffer()->GetI420()->StrideY() * _param->i_height;
			int u = frame.video_frame_buffer()->GetI420()->StrideU() * _param->i_height / 2;
			int v = frame.video_frame_buffer()->GetI420()->StrideV() * _param->i_height / 2;
			memcpy(pPic_in->img.plane[0], frame.video_frame_buffer()->GetI420()->DataY(), y);
			memcpy(pPic_in->img.plane[1], frame.video_frame_buffer()->GetI420()->DataU(), u);
			memcpy(pPic_in->img.plane[2], frame.video_frame_buffer()->GetI420()->DataV(), v);
			//memset(pPic_in->img.plane[2], 0x0, v);
			/*pPic_in->img.i_stride[0] = frame.video_frame_buffer()->GetI420()->StrideY();
			pPic_in->img.i_stride[1] = frame.video_frame_buffer()->GetI420()->StrideU();
			pPic_in->img.i_stride[2] = frame.video_frame_buffer()->GetI420()->StrideV();
			pPic_in->img.i_plane = 3;*/
			pPic_in->i_pts = counter;
			counter += 1;
			ret = x264_encoder_encode(_handle, &pNals, &iNal, pPic_in, pPic_out);
			if (ret < 0) {
				printf("Error.\n");
				break;
			}
			if (ret > 0)
			{
				for (int i = 0; i < iNal; ++i) 
				{
					//Ҫȥ��0x00 00 01 ���� 0x00 00 00 01
#if 0
					int off = 3;
					unsigned char* p = pNals[i].p_payload;
					if (p[0] == 0x00 && p[1] == 0x00
						&& p[2] == 0x00 && p[3] == 0x01)
					{
						off = 4;
					}
					if (pNals[i].i_payload > 1360)
					{
 						int a = 1;
					}
					//std::cout << "payload: " << pNals[i].i_payload - off << std::endl;
					rh->encode((const char*)pNals[i].p_payload + off, pNals[i].i_payload - off);
					rtp_packet_t* rtp;
					int rr = rh->get_packet(rtp);
					while (rr >= 0)
					{
						send_rtp(rtp, sockfd, ipstr, 12500);
						rr = rh->get_packet(rtp);
					}
#endif
					if (_cb)
					{
						_cb(pNals[i].p_payload, pNals[i].i_payload);
					}
				}
			}
#ifdef SAVEF
			if (ret > 0)
			{
				for (int i = 0; i < iNal; ++i) 
				{
					_h264_file->write((const char*)pNals[i].p_payload, pNals[i].i_payload);
				}
			}
#endif
		}
	}
}

void X264Encoder::send_rtp(rtp_packet_t* rtp, int fd, const char* ipstr, int port)
{
	int len = rtp_len(rtp);
	//std::cout << "send rtp, seqno = " << sockets::networkToHost16(rtp->hdr.seq_number) << "\tpayload len:" << rtp->payload_len << std::endl;
	uint8_t* buff = (uint8_t*)malloc(len);
	if (buff)
	{
		int ret = rtp_copy(rtp, buff, len);
		if (ret == 0)
		{
			sockets::SendUdpData(fd, ipstr, port, (const char*)buff, len);
			auto nalu = _rhd->decode_rtp(buff, len);
			if (nalu)
			{
				save_nalu(nalu);
			}
		}
		free(buff);
	}
	rtp_free(rtp);
}

void X264Encoder::save_nalu(NALU* nalu)
{
	NALU_HEADER* hdr = (NALU_HEADER*)nalu->payload;
	if (hdr->TYPE & NALU_TYPE_MASK == NALU_TYPE_IDR)
	{
		std::cout << "IDR slice..." << std::endl;
	}
#ifdef SAVEF
	_rtp_save->write((const char*)nalu->start_code, nalu->_start_code_len);
	_rtp_save->write((const char*)nalu->payload, nalu->payload_len);
#endif
	free(nalu->payload);
}
