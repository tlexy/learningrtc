#ifndef RTP_BASE_COMMON_DEF_H
#define RTP_BASE_COMMON_DEF_H

#include <stdint.h>

namespace rtp_base
{
	//扩展的rtp header
	struct rtc_ext_header
	{
		uint16_t ext_seq;
		uint8_t group_size;
		uint8_t group_seq;//for 0 to group_size-1
	};
}

#endif