#include "rtp_channel.h"
#include "../entity/jitter_buffer_entity.h"
#include "rtp_receiver.h"
#include <net_base/udp_channel.h>
#include "rtp_sender.h"

RtpChannel::RtpChannel(std::shared_ptr<uvcore::UdpServer> server, std::shared_ptr<JitterBufferEntity> entity)
	:_jb(entity),
	_receiver(std::make_shared<RtpReceiver>(entity)),
	_udp_channel(std::make_shared<UdpChannel>()),
	_udp_server(server)
{
	_sender = std::make_shared<RtpSender>(_udp_channel);
}

int RtpChannel::bind(const uvcore::IpAddress& ip)
{
	using namespace std::placeholders;
	return _udp_channel->bind(_udp_server, std::bind(&RtpReceiver::on_rtp_receive, _receiver.get(), _1, _2), ip);
}

int RtpChannel::set_remote_addr(const uvcore::IpAddress& raddr)
{
	return _udp_channel->set_remote_addr(raddr);
}

int RtpChannel::send(const char* data, int len)
{
	return _udp_channel->send(data, len);
}

void RtpChannel::set_rtp_param(uint8_t pt, uint32_t ssrc, uint32_t time_inter)
{
	_sender->set_rtp_param(pt, ssrc, time_inter);
}

void RtpChannel::send_rtp_packet(rtp_packet_t* rtp)
{
	_sender->send_rtp_packet(rtp);
}

void RtpChannel::send_raw_data(void* data, int len, uint32_t ts)
{
	_sender->send_raw_data(data, len, ts);
}