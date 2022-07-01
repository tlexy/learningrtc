#ifndef LEARNING_RTC_PC_COMMON_DEF_H
#define LEARNING_RTC_PC_COMMON_DEF_H

#include <any>

struct SignalHub
{
	int first;
	std::any t;
};

enum FirstSigType
{
	eSigJoinResp = 1,
	eSigVideoReady = 2,
	eSigFirstTypeEnd
};

class VideoParameter
{
public:
	int width;
	int height;
	int fps;
};

#endif