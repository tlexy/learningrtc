#ifndef LEARNING_RTC_RTP_RECEIVER_H
#define LEARNING_RTC_RTP_RECEIVER_H

#include <memory>
#include <functional>
#include <uvnet/core/udp_server.h>
#include <rtp_base/core/rtp.h>
#include <rtp_base/core/rtp_base_common_def.h>

class JitterBufferEntity;

class RtpReceiver
{
public:
	RtpReceiver(std::shared_ptr<JitterBufferEntity> entity);

	void on_rtp_receive(uvcore::Udp*, const struct sockaddr*);

private:
	uvcore::IpAddress _addr;
	std::shared_ptr<JitterBufferEntity> _je_entity;
};

#endif