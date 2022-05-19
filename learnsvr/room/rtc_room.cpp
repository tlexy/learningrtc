#include "rtc_room.h"

RtcRoom::RtcRoom(const std::string& appid, const std::string& roomid)
	:_appid(appid),
	_roomid(roomid)
{}

void RtcRoom::publish_stream(int64_t uid, const Json::Value& video_desc, const Json::Value& audio_desc)
{}

void RtcRoom::subscribe_stream(int64_t uid, int mask)
{}