#ifndef AAC_HELPER_H
#define AAC_HELPER_H

#include "aacenc_lib.h"
#include "aacdecoder_lib.h"
#include <stdint.h>
#include <common/audio/mid_buf.h>

class AacHelper 
{
public:
	AacHelper();
	//只能传入双通道
	bool openEncoder(int numOfChannels = 2, int sampleRate = 44100, int bitRate = 192000);
	int encode(const uint8_t* buf, int len, uint8_t* out_buf, int& out_len);

	static bool parse_aac_adts_head(uint8_t* adts_aac, int len, int& channel, int& bit_dep, int& sample_rate);

	bool openDecoder();
	bool decode(uint8_t* adts_aac, unsigned int len, uint8_t*& out_buff_addr, unsigned int& out_len);

private:
	HANDLE_AACENCODER _handle{NULL};
	mid_buf* _enc_buf{NULL};
	short* _inner_buf;

	int _out_buf_len{20480};
	uint8_t* _out_buf;

	//解码相关参数
	HANDLE_AACDECODER _d_handle{ NULL };
	int16_t* _dec_buf{ NULL };
	int _dec_buf_len{0};
};

#endif