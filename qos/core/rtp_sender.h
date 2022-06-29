#ifndef LEARNING_RTC_RTP_SENDER_H
#define LEARNING_RTC_RTP_SENDER_H

#include <memory>
#include <functional>
#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <stdint.h>
#include <rtp_base/core/rtp_base_common_def.h>
#include <rtp_base/core/rtp.h>
#include <list>

class JitterBufferEntity;

using SenderDataCb = std::function<void(uvcore::Udp*, const struct sockaddr*)>;

class RtpSender
{
public:
	struct RtpCachePacket
	{
		rtp_packet* rtp;
		void* buff;
		int len;
	};
	RtpSender(const uvcore::IpAddress& remote_addr, std::shared_ptr<JitterBufferEntity> entity,
		std::shared_ptr<uvcore::UdpServer> server);

	void set_data_cb(SenderDataCb cb);
	
	void enable_fec(uint8_t group_size, int redrate);
	//是否允许超时重发
	void enable_retrans(bool flag);
	void bind_local_addr(const uvcore::IpAddress& addr = uvcore::IpAddress());
	void set_rtp_param(uint8_t pt, uint32_t ssrc, uint32_t time_inter = 0);

	void send_rtp_packet(rtp_packet_t*);
	void send_raw_data(void* data, int len, uint32_t ts = 0);

private:
	RtpCachePacket* _pack_rtp(void* data, int len);
	RtpCachePacket* _pack_rtp(rtp_packet_t*);
	void free_rtpcache(RtpCachePacket*);
	void on_rtp_receive(uvcore::Udp*, const struct sockaddr*);

private:
	uvcore::IpAddress _remote_addr;
	uvcore::IpAddress _local_addr;
	SenderDataCb _data_cb{ nullptr };
	std::shared_ptr<JitterBufferEntity> _je_entity;
	std::shared_ptr<uvcore::UdpServer> _udp_server{ nullptr };
	uvcore::Udp* _rtp_udp{ nullptr };

	bool _retrans;

	rtp_session_t _sess;
	rtp_parameter_t _param;
	rtp_base::rtc_ext_header _rtc_header;

	std::list<RtpCachePacket*> _cache_list;
	int _max_cache_size{ 100 };

	uint32_t _interval{0};
	uint32_t _redrate{0};
};

#endif