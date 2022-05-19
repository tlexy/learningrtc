#include "room_manager.h"

RoomManager* const RoomManager::_instance = new RoomManager();

RoomManager::RoomManager()
{}

RoomManager* RoomManager::get_instance()
{
	return _instance;
}

std::shared_ptr<RtcRoom> RoomManager::find_room(const std::string& appid, const std::string& roomid)
{
	return nullptr;
}

std::shared_ptr<RtcRoom> RoomManager::create_room(const std::string& appid, const std::string& roomid)
{
	return nullptr;
}

//通过连接ID查找user
std::shared_ptr<RtcUser> RoomManager::find_user(int64_t connid)
{
	return nullptr;
}

std::shared_ptr<RtcUser> RoomManager::create_user(const std::string& appid, const std::string& roomid,
	int64_t uid, int64_t connid)
{
	return nullptr;
}