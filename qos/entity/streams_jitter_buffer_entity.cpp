#include "streams_jitter_buffer_entity.h"
#include "../core/rtp_cacher.h"
#include <endec/core/aac_helper.h>
#include <common/util/file_saver.h>
#include <video/test/h264_ffmpeg_decoder.h>
#include <iostream>

StreamsJitterBufferEntity::StreamsJitterBufferEntity()
{
	_h264_decoder = std::make_shared<H264FFmpegDecoder>();
}

void StreamsJitterBufferEntity::init()
{
	_rtp_cacher = std::make_shared<RtpCacher>();
	_rtp_cacher->set_update_cb(std::bind(&StreamsJitterBufferEntity::on_rtp_packet, this, std::placeholders::_1));

	_h264_decoder->init();
}

void StreamsJitterBufferEntity::push(rtp_packet_t* rtp)
{
	if (rtp->hdr.paytype == rtp_base::eAacLcPayLoad)
	{
		std::cout << "recv aac packet, seq: " << rtp->hdr.seq_number << std::endl;
		_rtp_cacher->push(rtp);
	}
	else if (rtp->hdr.paytype == rtp_base::eH264PayLoad)
	{
		std::cout << "recv h264 packet, seq: " << rtp->hdr.seq_number << std::endl;
		NALU* nalu = _rtp_h264_decoder->decode_rtp(rtp);
		if (nalu)
		{
			std::cout << "decode h264 packet, ts: " << rtp->hdr.timestamp << std::endl;
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
		//_pcm_buffer = std::make_shared<mid_buf>(_frame_size * ((_sample_rate + 1024) / 1024));

		_decode_buf_len = _frame_size * 3;
		_decode_buf = new uint8_t[_decode_buf_len];
	}
}

//不考虑网络丢包等情况，认为网络是理想的
bool StreamsJitterBufferEntity::force_cache()
{
	return false;
}

int StreamsJitterBufferEntity::get_pcm_buffer(int8_t* data, int len, int64_t& audio_pts)
{
	audio_pts = 0;
	int need_len = len;
	std::lock_guard<std::mutex> lock(_pcm_buffer_mutex);
	while (need_len > 0)
	{
		if (_pcm_buffers.empty())
		{
			break;
		}
		auto ptr = _pcm_buffers.front();
		if (need_len >= ptr->write_pos - ptr->read_pos)
		{
			//copy entire buffer
			memcpy(data, ptr->data + ptr->read_pos, ptr->write_pos - ptr->read_pos);
			need_len = need_len - (ptr->write_pos - ptr->read_pos);
			_pcm_buffers.pop_front();
			if (audio_pts == 0)
			{
				audio_pts = ptr->timestamp;
			}
		}
		else
		{
			//copy part of buffer
			memcpy(data, ptr->data + ptr->read_pos, need_len);
			need_len = 0;
			ptr->read_pos += need_len;
			if (audio_pts == 0)
			{
				audio_pts = ptr->timestamp;
			}
			break;
		}
	}
	return len - need_len;
}

int StreamsJitterBufferEntity::get_video_frame(AVFrame* frame, int64_t audio_pts)
{
	//根据音频开始时间戳、视频开始时间戳、当前音频时间、音频采样率及视频采样率，计算当前视频pts
	//audio_pts直接取自客户端传过来的rtp头部里的时间
	int64_t vts = get_video_pts(audio_pts);
	for (auto it = _frames.begin(); it != _frames.end(); ++it)
	{
		auto pit = it;
		++pit;
		if (pit != _frames.end())
		{
			if ((*it)->pts <= audio_pts && (*pit)->pts >= audio_pts)
			{
				frame = *it;
				_frames.erase(it);
				return 1;
			}
			else if ((*it)->pts > audio_pts)
			{
				return 0;
			}
		}
		else
		{
			if ((*it)->pts <= audio_pts)
			{
				frame = *it;
				_frames.erase(it);
				return 1;
			}
			return 0;
		}
	}
	return 0;
}

int64_t StreamsJitterBufferEntity::get_video_pts(int64_t audio_pts)
{
	//这里当audio_pts超出uint32_t时，要做重置处理
	//音频与视频的时间戳都从0开始
	int64_t audio_ts = audio_pts * 1000 / _sample_rate;
	int64_t video_ts = audio_ts * (90000 / 1000);
	return video_ts;
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
		auto ptr = std::make_shared<PcmBuffer>();
		ptr->data = (uint8_t*)malloc(_decode_buf_len);
		memcpy(ptr->data, _decode_buf, _decode_buf_len);
		/// Attention: 因为AAC解码器有比较大的解码延时，所以这个时间并不准确
		ptr->timestamp = rtp->hdr.timestamp;
		ptr->max_size = _decode_buf_len;
		ptr->write_pos = _decode_buf_len;
		_pcm_buffer_count += _decode_buf_len;
		_pcm_buffer_mutex.lock();
		//_pcm_buffer->push(_decode_buf, out_len);
		_pcm_buffers.push_back(ptr);
		if (_pcm_buffer_count >= _output_len)
		{
			_output_init = true;
		}
		_pcm_buffer_mutex.unlock();
	}
	rtp_free(rtp);

	/*_pcm_buffer_mutex.lock();
	if (_pcm_buffer->usable_count() >= _output_len)
	{
		_output_init = true;
	}
	_pcm_buffer_mutex.unlock();*/
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