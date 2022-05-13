#include "aac_helper.h"
#include <string.h>
#include <iostream>

#define __RET(ret) \
	if (ret != AACENC_OK) {\
		return false; \
	}

AacHelper::AacHelper()
{
	_enc_buf = new mid_buf(1024 * 1024);
}

bool AacHelper::openEncoder(int numOfChannels, int sampleRate, int bitRate)
{
	if (numOfChannels != 2)
	{
		return false;
	}
	HANDLE_AACENCODER hAacEncoder = NULL;
	AACENC_ERROR ret = aacEncOpen(&hAacEncoder, 0, 0);

	//很多参数要设置
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_AOT, AOT_AAC_LC);//2
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_BITRATE, bitRate);//192000
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_SBR_MODE, true);
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_SAMPLERATE, sampleRate);//44100
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_CHANNELMODE, MODE_2);
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_CHANNELORDER, 0);
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_AFTERBURNER, 0);
	__RET(ret)
	ret = aacEncoder_SetParam(hAacEncoder, AACENC_TRANSMUX, TT_MP4_ADTS);
	__RET(ret)
	ret = aacEncEncode(hAacEncoder, NULL, NULL, NULL, NULL);
	__RET(ret)

	//获取编码的信息
	AACENC_InfoStruct encInfo;
	aacEncInfo(hAacEncoder, &encInfo);

	//编码前存放数据的buffer
	_inner_buf = (short*)malloc(encInfo.frameLength * 2 * 2);
	_out_buf = (uint8_t*)malloc(_out_buf_len);

	//循环编码
	/*ErrorStatus = aacEncEncode(hAacEncoder,
		&inBufDesc,
		&outBufDesc,
		&inargs,
		&outargs);*/

	//关闭编码器
	//aacEncClose(&hAacEncoder);
	_handle = hAacEncoder;
	return true;
}

int AacHelper::encode(const uint8_t* buf, int len, uint8_t* out_buf, int& out_len)
{
	if (len != 4096)
	{
		std::cout << "input buffer error..." << std::endl;
		return 1;
	}
	AACENC_BufDesc inBufDesc;
	AACENC_BufDesc outBufDesc;
	AACENC_InArgs inargs;
	AACENC_OutArgs outargs;

	memset(&inBufDesc, 0x0, sizeof(AACENC_BufDesc));
	memset(&outBufDesc, 0x0, sizeof(AACENC_BufDesc));
	memset(&inargs, 0x0, sizeof(AACENC_InArgs));
	memset(&outargs, 0x0, sizeof(AACENC_OutArgs));

	const uint8_t* in = NULL;
	for (int i = 0; i < len / 2; ++i)
	{
		in = &buf[i << 1];
		_inner_buf[i] = in[0] | (in[1] << 8);
	}

	int in_identifier = IN_AUDIO_DATA;
	int in_size = len;
	int in_ele_size = 2;//2字节一个element?
	void* in_data = _inner_buf;
	inBufDesc.numBufs = 1;
	inBufDesc.bufs = &in_data;
	inBufDesc.bufSizes = &in_size;
	inBufDesc.bufElSizes = &in_ele_size;
	inBufDesc.bufferIdentifiers = &in_identifier;

	inargs.numInSamples = len / 2;

	int out_identifier = OUT_BITSTREAM_DATA;
	int out_size = 1;
	int out_ele_size = _out_buf_len;
	outBufDesc.numBufs = 1;
	outBufDesc.bufElSizes = &out_size;
	outBufDesc.bufferIdentifiers = &out_identifier;
	outBufDesc.bufSizes = &out_ele_size;
	outBufDesc.bufs = (void**)&_out_buf;

	AACENC_ERROR ret = aacEncEncode(_handle, &inBufDesc, &outBufDesc, &inargs, &outargs);
	if (ret == AACENC_OK && outargs.numOutBytes > 0 && out_buf != NULL && out_len >= outargs.numOutBytes)
	{
		memcpy(out_buf, _out_buf, outargs.numOutBytes);
		out_len = outargs.numOutBytes;
		return 0;
	}
	if (ret == AACENC_OK && outargs.numOutBytes <= 0)
	{
		out_len = 0;
		return 0;
	}
	else
	{
		return 2;//编码发生错误
	}
}