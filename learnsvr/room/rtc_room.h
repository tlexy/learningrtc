#ifndef LEARNING_RTC_ROOM_H
#define LEARNING_RTC_ROOM_H

#include <string>
#include <memory>
#include <jsoncpp/json/json.h>

class RtcUser;

class RtcRoom : public std::enable_shared_from_this<RtcRoom>
{
public:
	RtcRoom(const std::string& appid, const std::string& roomid);
	void add_user(std::shared_ptr<RtcUser> user);

	void publish_stream(int64_t uid, const Json::Value& video_desc, const Json::Value& audio_desc);
	void subscribe_stream(int64_t uid, int mask);

private:
	std::string _appid;
	std::string _roomid;
};

#endif