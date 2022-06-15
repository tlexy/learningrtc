#include "jetter_buffer_entity.h"
#include "../core/rtp_cacher.h"
//#include "../core/rtp_receiver.h"
#include <endec/core/aac_helper.h>
#include <common/util/file_saver.h>

JetterBufferEntity::JetterBufferEntity()
{
	
}

void JetterBufferEntity::init()
{
	_rtp_cacher = std::make_shared<RtpCacher>();
	_rtp_cacher->set_update_cb(std::bind(&JetterBufferEntity::on_rtp_packet, this, std::placeholders::_1));
}

//void JetterBufferEntity::start_recv(const uvcore::IpAddress& addr,
//	std::shared_ptr<uvcore::UdpServer> server)
//{
//	_rtp_receiver = std::make_shared<RtpReceiver>(addr, _rtp_cacher, server);
//	_rtp_receiver->start();
//
//	_rtp_cacher->set_update_cb(std::bind(&JetterBufferEntity::on_rtp_packet, this, std::placeholders::_1));
//}

void JetterBufferEntity::push(rtp_packet_t* rtp)
{
	_rtp_cacher->push(rtp);
}

bool JetterBufferEntity::force_cache()
{
	return false;
}

void JetterBufferEntity::update()
{
	bool flag = false;
	if (force_cache())
	{
		flag = true;
	}
	_rtp_cacher->update(flag);
}

void JetterBufferEntity::on_rtp_packet(rtp_packet_t* rtp)
{
	_mutex.lock();
	_rtp_list.push_back(rtp);
	_mutex.unlock();
}

void JetterBufferEntity::set_output_buffer(int64_t ms)
{
	_output_ms = ms;
}

void JetterBufferEntity::decode()
{
	do_decode();
}


///////////////////////////////AacJetterBufferEntity///////////////////--------------------------

AacJetterBufferEntity::AacJetterBufferEntity()
{
	_aac_helper = std::make_shared<AacHelper>();
	//_file_saver = new FileSaver(1024 * 1024, "receiver", ".pcm");
}

bool AacJetterBufferEntity::force_cache()
{
	if (!_is_init || !_output_init)
	{
		return false;
	}
	_pcm_buffer_mutex.lock();
	int pcm_buffer_rest = _pcm_buffer->usable_count();
	_pcm_buffer_mutex.unlock();

	if (pcm_buffer_rest == 0)
	{
		//要重新等待？
		_output_init = false;
	}
	//可用的数据太少时
	if (pcm_buffer_rest <= _frame_size)
	{
		return true;
	}

	return false;
}

int AacJetterBufferEntity::get_pcm_buffer(int8_t* data, int len)
{
	if (!_output_init)
	{
		return 0;
	}
	int usc = _pcm_buffer->usable_count();
	int min = usc > len ? len : usc;
	memcpy(data, _pcm_buffer->data(), min);
	_pcm_buffer->set_read(min);
	return min;
}

void AacJetterBufferEntity::aac_init()
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
		_output_len = ((_output_ms / frame_mill) + 1)* _frame_size;
		//最大1s
		_pcm_buffer = std::make_shared<mid_buf>(_frame_size *((_sample_rate + 1024)/1024));

		_decode_buf_len = _frame_size*3;
		_decode_buf = new uint8_t[_decode_buf_len];
	}
}

void AacJetterBufferEntity::do_decode()
{
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