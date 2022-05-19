#include "rtc_user.h"
#include "rtc_room.h"

RtcUser::RtcUser(const std::string& appid, const std::string& roomid, int64_t uid)
	:_appid(appid),
	_roomid(roomid),
	_uid(uid)
{}

void RtcUser::set_role(rtc::RtcRole role)
{
	_role = role;
}

void RtcUser::set_room(std::shared_ptr<RtcRoom> roomptr)
{
	_roomptr = roomptr;
}

void RtcUser::publish_stream(const Json::Value& desc)
{
	auto video_desc = desc["video"];
	auto audio_desc = desc["audio"];
	_roomptr->publish_stream(_uid, video_desc, audio_desc);
}