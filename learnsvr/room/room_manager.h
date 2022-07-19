#ifndef LEARNING_RTC_ROOM_MANAGER_H
#define LEARNING_RTC_ROOM_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <stdint.h>

class RtcRoom;
class RtcUser;

class RoomManager
{
public:
	using RoomList = std::unordered_map<std::string, std::shared_ptr<RtcRoom>>;
	using AllRoom = std::unordered_map<std::string, RoomList>;
	RoomManager();
	static RoomManager* get_instance();

	std::shared_ptr<RtcRoom> find_room(const std::string& appid, const std::string& roomid);
	std::shared_ptr<RtcRoom> create_room(const std::string& appid, const std::string& roomid);
	//通过连接ID查找user
	std::shared_ptr<RtcUser> find_user(int64_t connid);
	std::shared_ptr<RtcUser> create_user(const std::string& appid, const std::string& roomid,
		int64_t uid, int64_t connid);

private:
	static RoomManager* const _instance;
	std::unordered_map<int64_t, std::shared_ptr<RtcUser>> _all_users;
	AllRoom _all_rooms;
};

#endif