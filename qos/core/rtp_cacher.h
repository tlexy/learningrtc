#ifndef LEARNING_RTC_RTP_CACHER_H
#define LEARNING_RTC_RTP_CACHER_H

#include <rtp_base/core/rtp.h>
#include <rtp_base/core/rtp_base_common_def.h>
#include <list>
#include <functional>
#include <mutex>

using RtpCacherUpdateCb = std::function<void(rtp_packet_t*)>;

//是否需要区分audio cacher和video cacher？

class RtpCacher
{
public:
	//|0----p1-----------p2---|
	/*当包的个数多于p1时，update总会输出，并且update每次只输出一个包
	* 当包的个数多于p2时，不再接收新的包
	* */
	void push(rtp_packet_t*);
	void set_max_cache_size(int mcs);
	void update(bool force = false);
	void set_update_cb(RtpCacherUpdateCb);

private:
	std::mutex _mutex;
	int _failed_time{ 0 };
	RtpCacherUpdateCb _update_cb{ nullptr };
	std::list<rtp_packet_t*> _cache_list;
	int _max_cache_size{100};
	int _cache_size{ 10 };
	uint32_t _last_decode_seqno{0};//上一个送到解码器解码的包序号
};

#endif