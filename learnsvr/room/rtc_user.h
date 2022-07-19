#ifndef LEARNING_RTC_USER_H
#define LEARNING_RTC_USER_H

#include <string>
#include <stdint.h>
#include <common/def/rtc_common_def.h>
#include <memory>
#include <jsoncpp/json/json.h>

class RtcRoom;

class RtcUser
{
public:
	RtcUser(const std::string& appid, const std::string& roomid, int64_t uid);

	void set_role(rtc::RtcRole);
	void set_room(std::shared_ptr<RtcRoom> roomptr);
	void set_unique_id(int64_t unique);
	int64_t unique_id();

	int64_t uid();

	void publish_stream(const Json::Value& desc);

private:
	std::string _appid;
	std::string _roomid;
	int64_t _uid{0};
	int64_t _unique_id{0};
	rtc::RtcRole _role;
	std::shared_ptr<RtcRoom> _roomptr;
};

#endif