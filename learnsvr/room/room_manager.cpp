#include "room_manager.h"

RoomManager* const RoomManager::_instance = new RoomManager();

RoomManager::RoomManager()
{}

RoomManager* RoomManager::get_instance()
{
	return _instance;
}