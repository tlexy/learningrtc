#ifndef LEARNING_RTC_RTP_CACHER_H
#define LEARNING_RTC_RTP_CACHER_H

#include <rtp_base/core/rtp.h>
#include <rtp_base/core/rtp_base_common_def.h>
#include <list>

//是否需要区分audio cacher和video cacher？

class RtpCacher
{
public:

	void push(rtp_packet_t*);
	void set_max_cache_size(int mcs);

private:
	std::list<rtp_packet_t*> _cache_list;
	int _max_cache_size{100};
	uint32_t _last_decode_seqno{0};//上一个送到解码器解码的包序号
};

#endif