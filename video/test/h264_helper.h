// MPEG2RTP.h
#ifndef H264_HELPER_H
#define H264_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <video/rtp_h264/rtp_h264_def.h>

//查找开始字符0x000001
int FindStartCode2(unsigned char* Buf);

//查找开始字符0x00000001
int FindStartCode3(unsigned char* Buf);


//为NALU_t结构体分配内存空间
NALU* AllocNALU(int buffersize);

//释放
void FreeNALU(NALU* n);

//这个函数输入为一个NAL结构体，主要功能为得到一个完整的NALU并保存在NALU_t的buf中，获取他的长度，填充F,IDC,TYPE位。
//并且返回两个开始字符之间间隔的字节数，即包含有前缀的NALU的长度
int GetAnnexbNALU(NALU* nalu, FILE* fp);

int h264_decode_sps(uint8_t* buf, unsigned int len, int& width, int& height, int& fps);

#endif
