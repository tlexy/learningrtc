#ifndef LEARNING_RTC_PC_COMMON_DEF_H
#define LEARNING_RTC_PC_COMMON_DEF_H

#include <any>
extern "C" {
#include <libavformat/avformat.h>
}

struct SignalHub
{
	int first;
	std::any t;
};

enum FirstSigType
{
	eSigJoinResp = 1,
	eSigVideoReady = 2,
	eSigVideoFrame = 3,
	eSigFirstTypeEnd
};

class VideoParameter
{
public:
	int width;
	int height;
	int fps;
};

class VideoFrame
{
public:
	AVFrame* frame;
};

#endif