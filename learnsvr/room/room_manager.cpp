#include "room_manager.h"
#include "rtc_user.h"
#include "rtc_room.h"

RoomManager* const RoomManager::_instance = new RoomManager();

RoomManager::RoomManager()
{}

RoomManager* RoomManager::get_instance()
{
	return _instance;
}

std::shared_ptr<RtcRoom> RoomManager::find_room(const std::string& appid, const std::string& roomid)
{
	auto it = _all_rooms.find(appid);
	if (it == _all_rooms.end())
	{
		return nullptr;
	}
	auto sit = it->second.find(roomid);
	if (sit == it->second.end())
	{
		return nullptr;
	}
	return sit->second;
}

std::shared_ptr<RtcRoom> RoomManager::create_room(const std::string& appid, const std::string& roomid)
{
	return nullptr;
}

//通过连接ID查找user
std::shared_ptr<RtcUser> RoomManager::find_user(int64_t connid)
{
	auto it = _all_users.find(connid);
	if (it != _all_users.end())
	{
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<RtcUser> RoomManager::create_user(const std::string& appid, const std::string& roomid,
	int64_t uid, int64_t connid)
{
	auto user = std::make_shared<RtcUser>();
	if (!user)
	{
		return nullptr;
	}
	user->set_unique_id(connid);
	return user;
}