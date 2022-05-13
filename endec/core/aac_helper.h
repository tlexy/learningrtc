#ifndef AAC_HELPER_H
#define AAC_HELPER_H

#include "aacenc_lib.h"
#include <stdint.h>
#include <common/audio/mid_buf.h>

class AacHelper 
{
public:
	AacHelper();
	//只能传入双通道
	bool openEncoder(int numOfChannels = 2, int sampleRate = 44100, int bitRate = 192000);
	int encode(const uint8_t* buf, int len, uint8_t* out_buf, int& out_len);

	//生成ADTS头？本身自带ADTS头？

private:
	HANDLE_AACENCODER _handle{NULL};
	mid_buf* _enc_buf{NULL};
	short* _inner_buf;

	int _out_buf_len{20480};
	uint8_t* _out_buf;
};

#endif