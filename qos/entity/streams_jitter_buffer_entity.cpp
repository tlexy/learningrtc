#include "streams_jitter_buffer_entity.h"
#include "../core/rtp_cacher.h"
#include <endec/core/aac_helper.h>
#include <common/util/file_saver.h>
#include <video/test/h264_ffmpeg_decoder.h>

StreamsJitterBufferEntity::StreamsJitterBufferEntity()
{
	_h264_decoder = std::make_shared<H264FFmpegDecoder>();
}

void StreamsJitterBufferEntity::init()
{
	_rtp_cacher = std::make_shared<RtpCacher>();
	_rtp_cacher->set_update_cb(std::bind(&JitterBufferEntity::on_rtp_packet, this, std::placeholders::_1));

	_h264_decoder->init();
}

void StreamsJitterBufferEntity::push(rtp_packet_t* rtp)
{
	if (rtp->hdr.paytype == rtp_base::eAacLcPayLoad)
	{
		_rtp_cacher->push(rtp);
	}
	else if (rtp->hdr.paytype == rtp_base::eH264PayLoad)
	{
		NALU* nalu = _rtp_h264_decoder->decode_rtp(rtp);
		if (nalu)
		{
			nalu->timestamp = rtp->hdr.timestamp;
			_nalus_mutex.lock();
			_nalus.push_back(nalu);
			_nalus_mutex.unlock();
		}
	}
}

//void StreamsJitterBufferEntity::update()
//{}

void StreamsJitterBufferEntity::on_rtp_packet(rtp_packet_t* rtp)
{
	_mutex.lock();
	_rtp_list.push_back(rtp);
	_mutex.unlock();
}

void StreamsJitterBufferEntity::aac_init()
{
	//尝试获取音频并解析音频参数
	auto rtp = _rtp_cacher->front();
	if (rtp)
	{
		bool flag = _aac_helper->parse_aac_adts_head(rtp->arr, rtp->payload_len, _channel, _bit_dep, _sample_rate);
		if (!flag)
		{
			_channel = 0;
			return;
		}
		_is_init = true;
		_aac_helper->openDecoder();
		//根据设置的jetterbuffer ms设置接收的数据大小
		_frame_size = (_channel * _bit_dep * 1024) / 8; //一帧的大小
		int64_t seconds = (_channel * _bit_dep * _sample_rate) / 8;
		//一帧的时长
		int frame_mill = _sample_rate / 1024;
		//设置的_output_ms大概为多少帧 ？？？再乘以每帧的大小
		_output_len = ((_output_ms / frame_mill) + 1) * _frame_size;
		//最大1s
		_pcm_buffer = std::make_shared<mid_buf>(_frame_size * ((_sample_rate + 1024) / 1024));

		_decode_buf_len = _frame_size * 3;
		_decode_buf = new uint8_t[_decode_buf_len];
	}
}

//不考虑网络丢包等情况，认为网络是理想的
bool StreamsJitterBufferEntity::force_cache()
{
	return false;
}

void StreamsJitterBufferEntity::do_decode_aac()
{
	//解码aac，解码h264
	if (!_is_init)
	{
		aac_init();
		return;
	}
	if (!_aac_helper)
	{
		return;
	}
	//1. 从队列中拿到一个包
	_mutex.lock();
	if (_rtp_list.size() < 1)
	{
		_mutex.unlock();
		return;
	}
	auto rtp = _rtp_list.front();
	_rtp_list.pop_front();
	_mutex.unlock();

	//2. 送去解码
	unsigned out_len = 0;
	///to do... 这里还要设置丢包隐藏。。。
	auto flag = _aac_helper->decode(rtp->arr, rtp->payload_len, _decode_buf, out_len);
	if (flag)
	{
		_pcm_buffer_mutex.lock();
		_pcm_buffer->push(_decode_buf, out_len);
		_pcm_buffer_mutex.unlock();
	}
	rtp_free(rtp);

	_pcm_buffer_mutex.lock();
	if (_pcm_buffer->usable_count() >= _output_len)
	{
		_output_init = true;
	}
	_pcm_buffer_mutex.unlock();
}

void StreamsJitterBufferEntity::do_decode()
{
	do_decode_aac();
	do_decode_h264();
}

void StreamsJitterBufferEntity::do_decode_h264()
{
	_nalus_mutex.lock();
	for (auto it = _nalus.begin(); it != _nalus.end(); ++it)
	{
		_h264_decoder->decode(*it);
		free((*it)->payload);
		AVFrame* frame = nullptr;
		int ret = _h264_decoder->receive_frame(frame);
		while (ret >= 0)
		{
			if (ret > 0)
			{
				//有效帧
				_frames.push_back(frame);
			}
			frame = nullptr;
			ret = _h264_decoder->receive_frame(frame);
		}
	}
	_nalus_mutex.unlock();
}