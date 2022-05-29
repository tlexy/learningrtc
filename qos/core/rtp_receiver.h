﻿#ifndef LEARNING_RTC_RTP_RECEIVER_H
#define LEARNING_RTC_RTP_RECEIVER_H

#include <memory>
#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <rtp_base/core/rtp.h>
#include <rtp_base/core/rtp_base_common_def.h>

class JetterBufferEntity;

class RtpReceiver
{
public:
	RtpReceiver(const uvcore::IpAddress& addr, std::shared_ptr<JetterBufferEntity> entity,
		std::shared_ptr<uvcore::UdpServer> server);

	void start();

private:
	void on_rtp_receive(uvcore::Udp*, const struct sockaddr*);

private:
	uvcore::IpAddress _addr;
	std::shared_ptr<JetterBufferEntity> _je_entity;
	std::shared_ptr<uvcore::UdpServer> _udp_server{nullptr};
	uvcore::Udp* _rtp_udp{nullptr};
	bool _is_start{false};
};

#endif