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
	eSigFirstTypeEnd
};

#endif