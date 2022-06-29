// MPEG2RTP.h
#include <video/test/h264_helper.h>

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
