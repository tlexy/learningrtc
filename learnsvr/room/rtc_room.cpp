#include "rtc_room.h"
#include "rtc_user.h"

RtcRoom::RtcRoom(const std::string& appid, const std::string& roomid)
	:_appid(appid),
	_roomid(roomid)
{}

void RtcRoom::add_user(std::shared_ptr<RtcUser> user)
{
	user->set_room(shared_from_this());
}

void RtcRoom::publish_stream(int64_t uid, const Json::Value& video_desc, const Json::Value& audio_desc)
{}

void RtcRoom::subscribe_stream(int64_t uid, int mask)
{}