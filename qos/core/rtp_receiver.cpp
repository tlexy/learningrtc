#include "rtp_receiver.h"
#include <uvnet/core/udp.h>

RtpReceiver::RtpReceiver(const uvcore::IpAddress& addr, std::shared_ptr<RtpCacher> cacher,
	std::shared_ptr<uvcore::UdpServer> server)
	:_addr(addr),
	_rtp_cacher(cacher),
	_udp_server(server)
{
}

void RtpReceiver::start()
{
	using namespace std::placeholders;
	if (!_udp_server)
	{
		return;
	}
	_rtp_udp = _udp_server->addBind(_addr, std::bind(&RtpReceiver::on_rtp_receive, this, _1, _2));
}

void RtpReceiver::on_rtp_receive(uvcore::Udp*, const struct sockaddr*)
{
	//rtp验证与解包
	//每一个rtp包都包含一个rtp扩展头
}