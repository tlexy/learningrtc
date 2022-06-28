#include "rtp_sender.h"
#include <stdlib.h>
#include <uvnet/core/udp.h>
#include "../entity/jetter_buffer_entity.h"
#include <iostream>

RtpSender::RtpSender(const uvcore::IpAddress& remote_addr, std::shared_ptr<JetterBufferEntity> entity, 
	std::shared_ptr<uvcore::UdpServer> server)
	:_remote_addr(remote_addr),
	_udp_server(server),
	_je_entity(entity)
{
	_local_addr.setIp("0.0.0.0");
	_local_addr.setPort(0);

	_sess.seq_number = rand();
	_sess.timestamp = 0;// rand();

	_param.version = 2;
	_param.padding = 0;
	_param.ext = 1;
	_param.cc = 0;
	_param.marker = 0;

	_rtc_header.ext_seq = 0;
	_rtc_header.group_seq = 0;
}

void RtpSender::bind_local_addr(const uvcore::IpAddress& addr)
{
	using namespace std::placeholders;
	if (addr())
	{
		_local_addr = addr;
	}
	if (!_rtp_udp)
	{
		_rtp_udp = _udp_server->addBind(_local_addr, std::bind(&RtpSender::on_rtp_receive, this, _1, _2));
	}
}

void RtpSender::set_data_cb(SenderDataCb cb)
{
	_data_cb = cb;
}

void RtpSender::enable_fec(uint8_t group_size, int redrate)
{
	_redrate = redrate;
	_rtc_header.group_size = group_size;
	_rtc_header.red_size = (redrate * group_size) / 100;
}

void RtpSender::set_rtp_param(uint8_t pt, uint32_t ssrc, uint32_t time_inter)
{
	_interval = time_inter;
	_sess.ssrc = ssrc;
	_param.pt = pt;
}

void RtpSender::on_rtp_receive(uvcore::Udp* udp, const struct sockaddr* addr)
{
	if (_data_cb)
	{
		_data_cb(udp, addr);
	}
	auto rtp = rtp_unpack(udp->get_inner_buffer()->read_ptr(), udp->get_inner_buffer()->readable_size());
	if (rtp)
	{
		rtp_base::rtc_ext_header* rtc_header = (rtp_base::rtc_ext_header*)rtp->ext_body;
		uint32_t real_seqno = (rtc_header->ext_seq * 65535) + rtp->hdr.seq_number;
		std::cout << "receiver, receive rtp packet, rsn = " << real_seqno << std::endl;

		_je_entity->push(rtp);
	}
	udp->get_inner_buffer()->reset();
}

void RtpSender::enable_retrans(bool flag)
{
	_retrans = flag;
}

void RtpSender::send_rtp_packet(rtp_packet_t* rtp)
{
	if (!_rtp_udp)
	{
		return;
	}

	//准备发送
	auto rtpc = _pack_rtp(rtp);
	if (rtpc)
	{
		//发送了的包要缓存一断时间
		//_cache_list.push_back(rtpc);
		_rtp_udp->sendInLoop2((const char*)rtpc->buff, rtpc->len, _remote_addr);
		free_rtpcache(rtpc);
	}
}

void RtpSender::send_raw_data(void* data, int len, uint32_t ts)
{
	using namespace std::placeholders;
	/*if (!_rtp_udp)
	{
		_rtp_udp = _udp_server->addBind(_local_addr, std::bind(&RtpSender::on_rtp_receive, this, _1, _2));
	}*/
	if (!_rtp_udp)
	{
		return;
	}
	if (ts != 0)
	{
		_sess.timestamp = ts;
	}
	else
	{
		if (UINT32_MAX - _sess.timestamp > _interval)
		{
			_sess.timestamp += _interval;
		}
		else
		{
			_sess.timestamp = 0;
		}
	}

	if (_sess.seq_number >= UINT16_MAX)
	{
		_sess.seq_number = 0;
		++_rtc_header.ext_seq;
	}
	else
	{
		++_sess.seq_number;
	}
	if (_rtc_header.group_seq >= _rtc_header.group_size + _rtc_header.red_size)
	{
		_rtc_header.group_seq = 0;
	}
	else
	{
		++_rtc_header.group_seq;
	}
	//准备发送
	auto rtpc = _pack_rtp(data, len);
	if (rtpc)
	{
		//发送了的包要缓存一断时间
		//_cache_list.push_back(rtpc);
		_rtp_udp->sendInLoop2((const char*)rtpc->buff, rtpc->len, _remote_addr);
		free_rtpcache(rtpc);
	}
	//编码与发送冗余包
	if (_rtc_header.red_size > 0 && _rtc_header.group_seq == _rtc_header.group_size)
	{
		//to do...
	}
}

RtpSender::RtpCachePacket* RtpSender::_pack_rtp(rtp_packet_t* rtp)
{
	RtpCachePacket* rtpc = new RtpCachePacket();
	rtpc->len = rtp_len(rtp);
	rtpc->buff = malloc(rtpc->len);
	if (rtpc->buff)
	{
		int ret = rtp_copy(rtp, rtpc->buff, rtpc->len);
		if (ret == 0)
		{
			rtpc->rtp = rtp;
			return rtpc;
		}
	}
	return NULL;
}

RtpSender::RtpCachePacket* RtpSender::_pack_rtp(void* data, int len)
{
	RtpCachePacket* rtpc = new RtpCachePacket();
	if (!rtpc)
	{
		return NULL;
	}
	rtp_packet* rtp = rtp_alloc(len);
	rtp_pack(rtp, &_param, &_sess, data, len);

	rtp_copy_ext_hdr(rtp, rtp_base::eGroupExt, sizeof(rtp_base::rtc_ext_header), &_rtc_header);
	rtpc->len = rtp_len(rtp);
	rtpc->buff = malloc(rtpc->len);
	if (rtpc->buff)
	{
		int ret = rtp_copy(rtp, rtpc->buff, rtpc->len);
		if (ret == 0)
		{
			rtpc->rtp = rtp;
			return rtpc;
		}
	}
	return NULL;
}

void RtpSender::free_rtpcache(RtpCachePacket* rtpc)
{
	if (!rtpc)
	{
		return;
	}
	if (rtpc->buff)
	{
		free(rtpc->buff);
		rtpc->buff = NULL;
	}
	if (rtpc->rtp->ext_body)
	{
		free(rtpc->rtp->ext_body);
		rtpc->rtp->ext_body = NULL;
	}
	delete rtpc;
}