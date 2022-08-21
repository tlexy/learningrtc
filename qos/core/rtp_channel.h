#ifndef RTP_CHANNEL_H
#define RTP_CHANNEL_H

#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <rtp_base/core/rtp_base_common_def.h>
#include <rtp_base/core/rtp.h>

class JitterBufferEntity;
class RtpReceiver;
class UdpChannel;
class RtpSender;

class RtpChannel
{
public:
	RtpChannel(std::shared_ptr<uvcore::UdpServer> server, std::shared_ptr<JitterBufferEntity> entity);

	int bind(const uvcore::IpAddress& ip = uvcore::IpAddress());
	int set_remote_addr(const uvcore::IpAddress&);

	int send(const char* data, int len);

	//
	void set_rtp_param(uint8_t pt, uint32_t ssrc, uint32_t time_inter = 0);

	void send_rtp_packet(rtp_packet_t*);
	void send_raw_data(void* data, int len, uint32_t ts = 0);

private:
	std::shared_ptr<JitterBufferEntity> _jb;
	std::shared_ptr<RtpReceiver> _receiver;
	std::shared_ptr<UdpChannel> _udp_channel;
	std::shared_ptr<RtpSender> _sender;
	std::shared_ptr<uvcore::UdpServer> _udp_server;
};

#endif