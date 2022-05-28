#include "jetter_buffer_entity.h"
#include "../core/rtp_cacher.h"
#include "../core/rtp_receiver.h"
#include <endec/core/aac_helper.h>

JetterBufferEntity::JetterBufferEntity()
{
	_rtp_cacher = std::make_shared<RtpCacher>();
}

void JetterBufferEntity::start_recv(const uvcore::IpAddress& addr,
	std::shared_ptr<uvcore::UdpServer> server)
{
	_rtp_receiver = std::make_shared<RtpReceiver>(addr, _rtp_cacher, server);
	_rtp_receiver->start();

	_rtp_cacher->set_update_cb(std::bind(&JetterBufferEntity::on_rtp_packet, this, std::placeholders::_1));
}

bool JetterBufferEntity::force_cache()
{
	return false;
}

void JetterBufferEntity::update()
{
	if (_is_init)
	{
		_rtp_cacher->update();
	}
	else
	{
		if (force_cache())
		{
			_rtp_cacher->update(true);
		}
	}
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
}

bool AacJetterBufferEntity::force_cache()
{}

void AacJetterBufferEntity::do_decode()
{
	if (!_aac_helper)
	{
		return;
	}
	_mutex.lock();
	if (_rtp_list.size() < 1)
	{
		_mutex.unlock();
		return;
	}
	auto rtp = _rtp_list.front();
	_rtp_list.pop_front();
	_mutex.unlock();

	//解码。。。
	if (_channel == 0)
	{
		bool flag = _aac_helper->parse_aac_adts_head(rtp->arr, rtp->payload_len, _channel, _bit_dep, _sample_rate);
		if (!flag)
		{
			return;
		}
	}
	//怎么设置buffer大小等？
}