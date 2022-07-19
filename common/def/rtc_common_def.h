#ifndef LEARNING_RTC_COMMON_DEF_H
#define LEARNING_RTC_COMMON_DEF_H

#define AUDIO_MASK 0x1
#define VIDEO_MASK 0x2
#define AUDIO_VIDEO_MASK 0x3

namespace rtc
{
	enum ClientStat
	{
		InitStat = 0,
		WaitJoin = 1,
		WaitJoinResp = 2
	};

	enum RtcRole
	{
		RoleSubscriber = 100,
		RolePublisher = 101,
		RoleDataChannel = 102,
		RoleNull
	};
}

#endif