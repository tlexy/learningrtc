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

bool AacHelper::parse_aac_adts_head(uint8_t* adts_aac, int len, int& channel, int& bit_dep, int& sample_rate)
{
	if (len <= 7)
	{
		return false;
	}

	//syncword 12个1
	if ((adts_aac[0] == 0xFF) && ((adts_aac[1] & 0xF0) == 0xF0))
	{
		bool bGetParams = true;
		//ADTS中音频采样率索引取值情况
		//             96000, 88200, 64000, 48000, 44100, 32000,
		//            24000, 22050, 16000, 12000, 11025, 8000, 7350
		//             索引依次从0递增，44100的索引为4

		unsigned int sample_rate_tab[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };

		unsigned int syncword = (adts_aac[0] << 4) | (adts_aac[1] >> 4);
		unsigned int id = ((unsigned int)adts_aac[1] & 0x08) >> 3;
		unsigned int layer = ((unsigned int)adts_aac[1] & 0x06) >> 1;
		unsigned int protection_absent = (unsigned int)adts_aac[1] & 0x01;
		unsigned int profile = ((unsigned int)adts_aac[2] & 0xc0) >> 6;
		unsigned int source_samrate_index = ((unsigned int)adts_aac[2] & 0x3c) >> 2;
		if (source_samrate_index < sizeof(sample_rate_tab) / sizeof(sample_rate_tab[0]))
		{
			sample_rate = sample_rate_tab[source_samrate_index];
		}
		else
		{
			sample_rate = 0;
			bGetParams = false;
		}

		unsigned int private_bit = ((unsigned int)adts_aac[2] & 0x02) >> 1;
		channel = ((((unsigned int)adts_aac[2] & 0x01) << 2) | (((unsigned int)adts_aac[3] & 0xc0) >> 6));
		//当前支持2声道和单声道
		if ((channel != 1) && (channel != 2))
		{
			bGetParams = false;
			channel = 0;
		}

		//当前仅支持16bit位数
		bit_dep = 16;

		return bGetParams;
	}
	else
	{
		return false;
	}
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
	//if (len != 4096)
	//{
	//	std::cout << "input buffer error..." << std::endl;
	//	//return 1;
	//}
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

bool AacHelper::openDecoder()
{
	_d_handle = NULL;

	_d_handle = aacDecoder_Open(TT_MP4_ADTS, 1);
	if (_d_handle == NULL)
	{
		return false;
	}
	_dec_buf_len = 8 * 2 * 1024;
	_dec_buf = (int16_t*)malloc(_dec_buf_len);
	if (!_dec_buf)
	{
		return false;
	}
	return true;
}

bool AacHelper::decode(uint8_t* adts_aac, unsigned int len, uint8_t*& out_buff_addr, unsigned int& out_len)
{
	uint8_t* ptr = adts_aac;
	int i;
	unsigned int valid;
	unsigned int packet_size = len;
	AAC_DECODER_ERROR err;

	valid = packet_size;
	err = aacDecoder_Fill(_d_handle, &ptr, &packet_size, &valid);
	if (err != AAC_DEC_OK)
	{
		return false;
	}
	err = aacDecoder_DecodeFrame(_d_handle, _dec_buf, _dec_buf_len, 0);
	if (err == AAC_DEC_NOT_ENOUGH_BITS)
	{
		return true;
	}
	if (err != AAC_DEC_OK)
	{
		return false;
	}
	CStreamInfo* info = aacDecoder_GetStreamInfo(_d_handle);
	if (!info || info->sampleRate <= 0)
	{
		return false;
	}

	int frame_size = info->frameSize * info->numChannels;
	uint8_t* out = NULL;
	for (i = 0; i < frame_size; i++)
	{
		out = &out_buff_addr[i << 1]; // *2
		out[0] = _dec_buf[i] & 0xff;
		out[1] = _dec_buf[i] >> 8;
	}

	out_len = frame_size << 1;  // *2
	return true;
}