// MPEG2RTP.h
#include <video/test/h264_helper.h>
#include <cmath>

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//#include <video/rtp_h264/rtp_h264_def.h>
//
//typedef struct
//{
//  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
//  unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
//  unsigned max_size;            //! Nal Unit Buffer size
//  int forbidden_bit;            //! should be always FALSE
//  int nal_reference_idc;        //! NALU_PRIORITY_xxxx
//  int nal_unit_type;            //! NALU_TYPE_xxxx
//  unsigned char *buf;                    //! contains the first byte followed by the EBSP
//  unsigned short lost_packets;  //! true, if packet loss is detected
//} NALU_t;

//查找开始字符0x000001
int FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0; //判断是否为0x000001,如果是返回1
	else return 1;
}

//查找开始字符0x00000001
int FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//判断是否为0x00000001,如果是返回1
	else return 1;
}


//为NALU_t结构体分配内存空间
NALU *AllocNALU(int buffersize)
{
	NALU *n;

	if ((n = (NALU*)calloc (1, sizeof (NALU))) == NULL)
	{
		printf("AllocNALU: n");
		exit(0);
	}

	n->max_size=buffersize;

	if ((n->payload = (unsigned char*)calloc (buffersize, sizeof (char))) == NULL) //默认80000字节的大小，太吓人了
	{
		free (n);
		printf ("AllocNALU: n->buf");
		exit(0);
	}
	if ((n->hdr = (NALU_HEADER*)calloc(1, sizeof(char))) == NULL) //默认80000字节的大小，太吓人了
	{
		free(n);
		printf("AllocNALU: n->buf");
		exit(0);
	}

	return n;
}


//释放
void FreeNALU(NALU *n)
{
	if (n)
	{
		if (n->payload)
		{
			free(n->payload);
			n->payload=NULL;
		}
		if (n->hdr)
		{
			free(n->hdr);
			n->hdr = NULL;
		}
		free (n);
	}
}



//这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，获取他的长度，填充F,IDC,TYPE位。
//并且返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
int GetAnnexbNALU (NALU *nalu, FILE* fp)
{
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
		printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

	nalu->start_code_len =3;//初始化码流序列的开始字符为3个字节

	if (3 != fread (Buf, 1, 3, fp))//从码流中读3个字节
	{
		free(Buf);
		return 0;
	}
	int ret = FindStartCode2 (Buf);//判断是否为0x000001 
	if(ret != 1)
	{
		//如果不是，再读一个字节
		if(1 != fread(Buf+3, 1, 1, fp))//读一个字节
		{
			free(Buf);
			return 0;
		}
		ret = FindStartCode3 (Buf);//判断是否为0x00000001
		if (ret != 1)//如果不是，返回-1
		{ 
			free(Buf);
			return -1;
		}
		else 
		{
			//如果是0x00000001,得到开始前缀为4个字节
			pos = 4;
			nalu->start_code_len = 4;
		}
	}
	else
	{
		//如果是0x000001,得到开始前缀为3个字节
		nalu->start_code_len = 3;
		pos = 3;
	}
	//查找下一个开始字符的标志位
	StartCodeFound = 0;
	int ret2 = 0;

	while (!StartCodeFound)
	{
		if (feof (fp))//判断是否到了文件尾
		{
			nalu->payload_len = (pos-1)-nalu->start_code_len;
			//memcpy (nalu->payload, &Buf[nalu->start_code_len], nalu->payload_len);
			//nalu->hdr->F = nalu->payload[0] & 0x80;
			//nalu->hdr->NRI = nalu->payload[0] & 0x60;  // 2 bit
			//nalu->hdr->TYPE = (nalu->payload[0]) & 0x1f;    // 5 bit

			memcpy(nalu->payload, &Buf[nalu->start_code_len], nalu->payload_len);
			nalu->hdr->F = nalu->payload[0] & 0x80;
			nalu->hdr->NRI = nalu->payload[0] & 0x60;  // 2 bit
			nalu->hdr->TYPE = (nalu->payload[0]) & 0x1f;    // 5 bit
			free(Buf);
			return pos-1;
		}
		Buf[pos++] = fgetc (fp);//读一个字节到BUF中
		int ret2 = FindStartCode3(&Buf[pos-4]);//判断是否为0x00000001
		if(ret2 != 1)
			ret = FindStartCode2(&Buf[pos-3]);//判断是否为0x000001
		StartCodeFound = (ret == 1 || ret2 == 1);
	}



	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (ret2 == 1)? -4 : -3;

	if (0 != fseek (fp, rewind, SEEK_CUR))//把文件指针指向前一个NALU的末尾
	{
		free(Buf);
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
		return  0;
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	nalu->payload_len = (pos + rewind);// -nalu->start_code_len;
	memcpy (nalu->payload, Buf, nalu->payload_len + nalu->start_code_len);//拷贝一个完整NALU
	nalu->hdr->F = nalu->payload[nalu->start_code_len] & 0x80;
	nalu->hdr->NRI = nalu->payload[nalu->start_code_len] & 0x60;  // 2 bit
	nalu->hdr->TYPE = (nalu->payload[nalu->start_code_len]) & 0x1f;    // 5 bit
	free(Buf);

	return (pos+rewind);//返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
}

uint32_t Ue(uint8_t* pBuff, uint32_t nLen, uint32_t& nStartBit)
{
	//计算0bit的个数
	uint32_t nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;


	//计算结果
	unsigned long dwRet = 0;
	for (uint32_t i = 0; i < nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}


int Se(uint8_t* pBuff, uint32_t nLen, uint32_t& nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}


int64_t u(uint32_t BitCount, uint8_t* buf, uint32_t& nStartBit)
{
	int64_t dwRet = 0;
	for (uint32_t i = 0; i < BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

void de_emulation_prevention(uint8_t* buf, unsigned int* buf_size)
{
	int i = 0, j = 0;
	uint8_t* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i < (tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j < tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}

int h264_decode_sps(uint8_t* buf, unsigned int nLen, int& width, int& height, int& fps)
{
	uint32_t StartBit = 0;
	fps = 0;
	de_emulation_prevention(buf, &nLen);

	int forbidden_zero_bit = u(1, buf, StartBit);
	int nal_ref_idc = u(2, buf, StartBit);
	int nal_unit_type = u(5, buf, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, buf, StartBit);
		int constraint_set0_flag = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, buf, StartBit);
		int level_idc = u(8, buf, StartBit);

		int seq_parameter_set_id = Ue(buf, nLen, StartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			int chroma_format_idc = Ue(buf, nLen, StartBit);
			if (chroma_format_idc == 3)
				int residual_colour_transform_flag = u(1, buf, StartBit);
			int bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
			int bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
			int seq_scaling_matrix_present_flag = u(1, buf, StartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
		int pic_order_cnt_type = Ue(buf, nLen, StartBit);
		if (pic_order_cnt_type == 0)
			int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, buf, StartBit);
			int offset_for_non_ref_pic = Se(buf, nLen, StartBit);
			int offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

			int* offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
			delete[] offset_for_ref_frame;
		}
		int num_ref_frames = Ue(buf, nLen, StartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
		int pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
		int pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

		//width=(pic_width_in_mbs_minus1+1)*16;
		//height=(pic_height_in_map_units_minus1+1)*16;

		int frame_mbs_only_flag = u(1, buf, StartBit);
		if (!frame_mbs_only_flag)
			int mb_adaptive_frame_field_flag = u(1, buf, StartBit);
		int direct_8x8_inference_flag = u(1, buf, StartBit);
		int frame_cropping_flag = u(1, buf, StartBit);
		int frame_crop_left_offset = 0;
		int frame_crop_right_offset = 0;
		int frame_crop_top_offset = 0;
		int frame_crop_bottom_offset = 0;
		if (frame_cropping_flag)
		{
			frame_crop_left_offset = Ue(buf, nLen, StartBit);
			frame_crop_right_offset = Ue(buf, nLen, StartBit);
			frame_crop_top_offset = Ue(buf, nLen, StartBit);
			frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
		}

		width = ((pic_width_in_mbs_minus1 + 1) * 16) - frame_crop_left_offset * 2 - frame_crop_right_offset * 2;
		height = ((2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);

		int vui_parameter_present_flag = u(1, buf, StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, buf, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, buf, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, buf, StartBit);
					int sar_height = u(16, buf, StartBit);
				}
			}
			int overscan_info_present_flag = u(1, buf, StartBit);
			if (overscan_info_present_flag)
				int overscan_appropriate_flagu = u(1, buf, StartBit);
			int video_signal_type_present_flag = u(1, buf, StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, buf, StartBit);
				int video_full_range_flag = u(1, buf, StartBit);
				int colour_description_present_flag = u(1, buf, StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, buf, StartBit);
					int transfer_characteristics = u(8, buf, StartBit);
					int matrix_coefficients = u(8, buf, StartBit);
				}
			}
			int chroma_loc_info_present_flag = u(1, buf, StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
				int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
			}
			int timing_info_present_flag = u(1, buf, StartBit);
			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, buf, StartBit);
				int time_scale = u(32, buf, StartBit);
				fps = time_scale / (2 * num_units_in_tick);
			}
		}
		return true;
	}
	else
	return false;
}
