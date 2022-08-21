#include "rtp_receiver.h"
#include <uvnet/core/udp.h>
#include "rtp_cacher.h"
#include "../entity/jitter_buffer_entity.h"
#include <iostream>

RtpReceiver::RtpReceiver(const uvcore::IpAddress& addr, std::shared_ptr<JitterBufferEntity> entity,
	std::shared_ptr<uvcore::UdpServer> server)
	:_addr(addr),
	_je_entity(entity),
	_udp_server(server)
{
}

void RtpReceiver::start()
{
	using namespace std::placeholders;
	if (!_udp_server)
	{
		std::cout << "bind udp error." << std::endl;
		return;
	}
	std::cout << "bind udp success." << std::endl;
	_rtp_udp = _udp_server->addBind(_addr, std::bind(&RtpReceiver::on_rtp_receive, this, _1, _2));
}

void RtpReceiver::set_data_cb(ReceiverDataCb cb)
{
	_data_cb = cb;
}

void RtpReceiver::on_rtp_receive(uvcore::Udp* udp, const struct sockaddr* addr)
{
	if (_data_cb)
	{
		_data_cb(udp, addr);
	}
	//rtp验证与解包
	//每一个rtp包都包含一个rtp扩展头
	auto rtp = rtp_unpack(udp->get_inner_buffer()->read_ptr(), udp->get_inner_buffer()->readable_size());
	if (!rtp)
	{
		udp->get_inner_buffer()->reset();
		return;
	}
	rtp_base::rtc_ext_header* rtc_header = (rtp_base::rtc_ext_header*)rtp->ext_body;
	uint32_t real_seqno = (rtc_header->ext_seq*65535) + rtp->hdr.seq_number;
	//首先判断包是不是不再需要的包？？？
	
	//怎么判断一组包已经完全到达或者FEC可解包
	//当一组包完全效给上一给时，后续到达的FEC包或者正常的内容包，都要丢弃
	if (rtp->hdr.paytype == rtp_base::eFecPayLoad)
	{
		uint32_t min_seqno = real_seqno - rtc_header->group_size;
		//1. 查找[min_seqno, real_seqno]范围内的包用于FEC解码
		//2. FEC解码完成后，将丢失（未到达）的包上交到上一层
		//3. [min_seqno, real_seqno]范围内的包接收到就放弃
	}
	else if (rtp->hdr.paytype == rtp_base::eAacLcPayLoad
		|| rtp->hdr.paytype == rtp_base::eH264PayLoad)
	{
		//std::cout << "receiver, receive rtp packet, rsn = " << real_seqno << std::endl;
		_je_entity->push(rtp);
	}

	udp->get_inner_buffer()->reset();
}