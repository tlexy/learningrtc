#include "rtp_h264_decoder.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <video/test/h264_helper.h>

RtpH264Decoder::RtpH264Decoder()
{
}

NALU* RtpH264Decoder::decode_rtp(const uint8_t* payload, int len, bool with_startcode)
{
	if (!rtp_unpack_test((void*)payload, len))
	{
		return nullptr;
	}
	int ptype = rtp_payload_type((void*)payload, len);
	if (ptype != rtp_base::eH264PayLoad)
	{
		return nullptr;
	}
	rtp_packet_t* rtp = rtp_unpack((void*)payload, len);
	//std::cout << "recv rtp, seqno = " << rtp->hdr.seq_number << "\tpayload len:" << rtp->payload_len << std::endl;
	NALU_HEADER* hdr = (NALU_HEADER*)rtp->arr;
	if (hdr->TYPE != 28)
	{
		_last_seqno = rtp->hdr.seq_number;
		_last_ts = rtp->hdr.timestamp;
		return decode_single(rtp, with_startcode);
	}
	else
	{
		std::cout << "decode fua..." << std::endl;
		return decode_fua(rtp, with_startcode);
	}
}

NALU* RtpH264Decoder::decode_rtp(rtp_packet_t* rtp, bool with_startcode)
{
	NALU_HEADER* hdr = (NALU_HEADER*)rtp->arr;
	if (hdr->TYPE != 28)
	{
		_last_seqno = rtp->hdr.seq_number;
		_last_ts = rtp->hdr.timestamp;
		NALU* nalu = decode_single(rtp, with_startcode);
		rtp_free(rtp);
		return nalu;
	}
	else
	{
		std::cout << "decode real fua..." << std::endl;
		return decode_fua(rtp, with_startcode);
	}
}

NALU* RtpH264Decoder::decode_fua(rtp_packet_t* rtp, bool with_startcode)
{
	/*如果和上个包的序号相差为1，正常接收；大于1，那么就要放弃整个帧
	* 将接收的包放到一个链表中，当接收到最后一个时，进行恢复
	*/
	if (is_first_fua(rtp))
	{
		_last_ts = rtp->hdr.timestamp;
		_fu_list.push_back(rtp);
		_last_seqno = rtp->hdr.seq_number;
		_start_seqno = _last_seqno;
	}
	else
	{
		//如果不是同一时间或者收到迟到的包
		//如果不是第一个包，那么链表中至少有一个包存在
		if (_last_ts != rtp->hdr.timestamp
			|| rtp->hdr.seq_number < _last_seqno
			|| _fu_list.size() == 0)
		{
			std::cout << "decode_fua free all" << std::endl;
			rtp_free(rtp);
			//丢弃所有收到的fua包
			for (auto it = _fu_list.begin(); it != _fu_list.end(); ++it)
			{
				rtp_free(*it);
			}
			_fu_list.clear();
			return nullptr;
		}
		if (rtp->hdr.seq_number > _last_seqno && rtp->hdr.seq_number - _last_seqno == 1)
		{
			_fu_list.push_back(rtp);
			_last_seqno = rtp->hdr.seq_number;
		}
		if (is_last_fua(rtp))
		{
			//std::cout << "seq diff: " << _last_seqno - _start_seqno << std::endl;
			//输出（重新整合）整个NALU包
			auto nalu = assembly_nalu(_fu_list, with_startcode);
			nalu->timestamp = rtp->hdr.timestamp;
			std::cout << "fua len: " << nalu->payload_len << std::endl;
			for (auto it = _fu_list.begin(); it != _fu_list.end(); ++it)
			{
				rtp_free(*it);
			}
			_fu_list.clear();
			return nalu;
		}
	}
	return nullptr;
}

bool RtpH264Decoder::is_first_fua(rtp_packet_t* rtp)
{
	if (rtp_len(rtp) <= 2)
	{
		return false;
	}
	FU_INDICATOR* idc = (FU_INDICATOR*)rtp->arr;
	FU_HEADER* hdr = (FU_HEADER*)(rtp->arr + 1);
	if (hdr->S == 1 && hdr->R == 0 && hdr->E == 0)
	{
		return true;
	}
	return false;
}

bool RtpH264Decoder::is_last_fua(rtp_packet_t* rtp)
{
	if (rtp_len(rtp) <= 2)
	{
		return false;
	}
	FU_HEADER* hdr = (FU_HEADER*)(rtp->arr + 1);
	if (hdr->S == 0 && hdr->R == 0 && hdr->E == 1)
	{
		return true;
	}
	return false;
}

NALU* RtpH264Decoder::assembly_nalu(const std::list<rtp_packet_t*>&, bool with_startcode)
{
	if (_fu_list.size() < 2)
	{
		_fu_list.clear();
		return nullptr;
	}
	int total_len = 0;
	for (auto it = _fu_list.begin(); it != _fu_list.end(); ++it)
	{
		total_len = total_len + (*it)->payload_len - 2;
	}
	NALU* nalu = new NALU;
	if (!nalu)
	{
		return nullptr;
	}
	memset(nalu, 0x0, sizeof(NALU));
	nalu->start_code[0] = 0x0;
	nalu->start_code[1] = 0x0;
	nalu->start_code[2] = 0x01;
	nalu->start_code[3] = 0x01;
	nalu->start_code_len = 3;
	if (!with_startcode)
	{
		nalu->payload_len = total_len + 1;
	}
	else
	{
		nalu->payload_len = total_len + 1 + nalu->start_code_len;
	}
	nalu->payload = (uint8_t*)malloc(nalu->payload_len);
	if (!nalu->payload)
	{
		return nullptr;
	}
	FU_INDICATOR* idc = (FU_INDICATOR*)(_fu_list.front()->arr);
	FU_HEADER* hdr = (FU_HEADER*)(_fu_list.front()->arr + 1);
	
	int off = 1;
	if (with_startcode)
	{
		memcpy(nalu->payload, nalu->start_code, nalu->start_code_len);
		off += nalu->start_code_len;
	}
	for (auto it = _fu_list.begin(); it != _fu_list.end(); ++it)
	{
		memcpy(nalu->payload + off, (*it)->arr + 2, (*it)->payload_len - 2);
		off += ((*it)->payload_len - 2);
	}

	if (!with_startcode)
	{
		nalu->hdr = (NALU_HEADER*)nalu->payload;
	}
	else
	{
		nalu->hdr = (NALU_HEADER*)(nalu->payload + nalu->start_code_len);
	}
	nalu->hdr->F = 0;
	nalu->hdr->NRI = idc->NRI;
	nalu->hdr->TYPE = hdr->TYPE;
	//nalu->timestamp = _last_ts;

	return nalu;
}

NALU* RtpH264Decoder::decode_single(rtp_packet_t* rtp, bool with_startcode)
{
	NALU_HEADER* hdr = (NALU_HEADER*)rtp->arr;
	NALU* nalu = new NALU;
	if (!nalu)
	{
		return nullptr;
	}
	memset(nalu, 0x0, sizeof(NALU));
	nalu->start_code[0] = 0x0;
	nalu->start_code[1] = 0x0;
	nalu->start_code[2] = 0x01;
	nalu->start_code[3] = 0x01;
	nalu->start_code_len = 3;
	if ((hdr->TYPE & NALU_TYPE_MASK) == NALU_TYPE_SPS
		|| (hdr->TYPE & NALU_TYPE_MASK) == NALU_TYPE_PPS)
	{
		nalu->start_code[2] = 0x00;
		nalu->start_code_len = 4;
	}
	std::cout << "decode single..." << std::endl;
	//假设只有拆分包，没有组合包
	if (hdr->TYPE != 28)
	{
		if (!with_startcode)
		{
			nalu->payload_len = rtp->payload_len;
			nalu->payload = (uint8_t*)malloc(nalu->payload_len);
			memcpy(nalu->payload, rtp->arr, nalu->payload_len);
			nalu->hdr = (NALU_HEADER*)nalu->payload;
		}
		else
		{
			nalu->payload_len = rtp->payload_len + nalu->start_code_len;
			nalu->payload = (uint8_t*)malloc(nalu->payload_len);
			memcpy(nalu->payload, nalu->start_code, nalu->start_code_len);
			memcpy(nalu->payload + nalu->start_code_len, rtp->arr, nalu->payload_len - nalu->start_code_len);
			nalu->hdr = (NALU_HEADER*)(nalu->payload + nalu->start_code_len);
		}
		nalu->timestamp = rtp->hdr.timestamp;

		//下面的操作会修改h264的内容，要放在最后
		if ((hdr->TYPE & NALU_TYPE_MASK) == NALU_TYPE_SPS)
		{
			int width = 0, height = 0, fps = 0;
			h264_decode_sps(rtp->arr, rtp->payload_len, width, height, fps);
			nalu->width = width;
			nalu->height = height;
			nalu->fps = fps;
		}

		return nalu;
	}
	std::cerr << "decode nalu error." << std::endl;
	return nullptr;
}