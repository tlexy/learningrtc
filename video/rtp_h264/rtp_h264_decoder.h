#ifndef RTP_H264_DECODER_H
#define RTP_H264_DECODER_H

#include "rtp_h264_def.h"
#include <rtp_base/core/rtp_base_common_def.h>
#include <rtp_base/core/rtp.h>
#include <list>

class RtpH264Decoder
{
public:
	RtpH264Decoder();
	
	//假设rtp包是按序到达的，只是可能存在丢包
	NALU* decode_rtp(const uint8_t* payload, int len, bool with_startcode = false);
	NALU* decode_rtp(rtp_packet_t* rtp, bool with_startcode = false);

private:
	NALU* decode_single(rtp_packet_t*, bool with_startcode = false);
	NALU* decode_fua(rtp_packet_t*, bool with_startcode = false);

	bool is_first_fua(rtp_packet_t* rtp);
	bool is_last_fua(rtp_packet_t* rtp);
	NALU* assembly_nalu(const std::list<rtp_packet_t*>&, bool with_startcode = false);

private:
	uint16_t _last_seqno{0};
	uint16_t _start_seqno{ 0 };
	uint32_t _last_ts{ 0 };
	std::list<rtp_packet_t*> _fu_list;
	uint32_t _drop_ts{ 0 };
};


#endif