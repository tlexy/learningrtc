﻿#include "rtp_cacher.h"

void RtpCacher::push(rtp_packet_t* rtp)
{
	if (_cache_list.size() == 0)
	{
		_cache_list.push_back(rtp);
		return;
	}
	if (_cache_list.size() >= _max_cache_size)
	{
		rtp_free(rtp);
		return;
	}
	rtp_base::rtc_ext_header* rtc_header = (rtp_base::rtc_ext_header*)rtp->ext_body;
	uint32_t rsn = (rtc_header->ext_seq * 65535) + rtp->hdr.seq_number;

	//最后一个项
	rtc_header = (rtp_base::rtc_ext_header*)_cache_list.back()->ext_body;
	uint32_t last_rsn = (rtc_header->ext_seq * 65535) + _cache_list.back()->hdr.seq_number;
	if (rsn > last_rsn)
	{
		_cache_list.push_back(rtp);
		return;
	}
	//是否比第一个项还要小？
	rtc_header = (rtp_base::rtc_ext_header*)_cache_list.front()->ext_body;
	uint32_t first_rsn = (rtc_header->ext_seq * 65535) + _cache_list.front()->hdr.seq_number;
	if (first_rsn >= rsn)
	{
		rtp_free(rtp);
		return;
	}

	uint32_t c_rsn = 0;
	//迟到的包，插入到适当的位置 
	auto it = _cache_list.begin();
	for (; it != _cache_list.end(); ++it)
	{
		rtc_header = (rtp_base::rtc_ext_header*)(*it)->ext_body;
		c_rsn = (rtc_header->ext_seq * 65535) + (*it)->hdr.seq_number;
		if (c_rsn == rsn)
		{
			rtp_free(rtp);
			return;
		}
		//找到第一个比传进来的real seqno要大的项，插入到这个项的前面
		if (c_rsn > rsn)
		{
			break;
		}
	}
	if (it != _cache_list.end())
	{
		_cache_list.push_back(rtp);
	}
}

void RtpCacher::set_max_cache_size(int mcs)
{
	_max_cache_size = mcs;
}