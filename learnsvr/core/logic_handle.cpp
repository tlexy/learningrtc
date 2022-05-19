#include "logic_handle.h"
#include <common/util/sutil.h>
#include <log4u/core/common_log.h>
#include <uvnet/core/tcp_connection.h>
#include "../room/room_manager.h"
#include "../room/rtc_room.h"
#include "../room/rtc_user.h"
#include <uvnet/core/packet_helpers.h>

#define REG_HANDLE(text, handle) _func_map[text] = std::bind(&RtcLogicHandle::handle, this, _1)

RtcLogicHandle* const RtcLogicHandle::_instance = new RtcLogicHandle();

RtcLogicHandle::RtcLogicHandle()
{}

RtcLogicHandle* RtcLogicHandle::get_instance()
{
	return _instance;
}

void RtcLogicHandle::init()
{
	using namespace std::placeholders;
	//_func_map["Join"] = std::bind(&RtcLogicHandle::handle_join, this, _1);
	REG_HANDLE("Join", handle_join);
}

void RtcLogicHandle::handle_msg(const std::string& buff, std::shared_ptr<uvcore::TcpConnection> ptr)
{
	auto json = std::make_shared<Json::Value>();
	bool flag = SUtil::parseJson(buff.c_str(), *json);
	if (!flag)
	{
		log_info("parse json data error");
		return;
	}

	auto unit = std::make_shared<MsgUnit>();
	unit->action = (*json)["action"].asString();
	std::cout << "action: " << unit->action << std::endl;
	if (_func_map.find(unit->action) != _func_map.end())
	{
		unit->json = json;
		unit->conn = ptr;
		std::lock_guard<std::mutex> lock(_queue_mutex);
		_msg_queue.push_back(unit);
	}
}

void RtcLogicHandle::update()
{
	int conti = 3;
	while (!_is_thread_stop)
	{
		if (conti == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		std::list<std::shared_ptr<MsgUnit>> tmp_list;
		{
			//尝试获取数据
			std::lock_guard<std::mutex> lock(_queue_mutex);
			if (_msg_queue.size() <= 0)
			{
				if (conti > 0)
				{
					--conti;
				}
				continue;
			}
			conti = 3;
			tmp_list.swap(_msg_queue);
		}

		//查找并处理
		for (auto it = tmp_list.begin(); it != tmp_list.end(); ++it)
		{
			auto func_it = _func_map.find((*it)->action);
			if (func_it != _func_map.end())
			{
				func_it->second((*it));
			}
		}
	}
}

void RtcLogicHandle::start_thread()
{
	if (_th)
	{
		return;
	}
	_is_thread_stop = false;
	_th = std::make_shared<std::thread>(&RtcLogicHandle::update, this);
}

void RtcLogicHandle::stop_thread()
{
	_is_thread_stop = true;
	if (_th && _th->joinable())
	{
		_th->join();
		_th = nullptr;
	}
}

void RtcLogicHandle::handle_join(std::shared_ptr<MsgUnit> unit)
{
	std::string appid = (*unit->json)["appid"].asString();
	std::string roomid = (*unit->json)["roomid"].asString();
	int64_t uid = (*unit->json)["uid"].asInt64();
	
	/*auto roomptr = RoomManager::get_instance()->find_room(appid, roomid);
	if (!roomptr)
	{
		roomptr = RoomManager::get_instance()->create_room(appid, roomid);
	}

	auto user = RoomManager::get_instance()->find_user(unit->conn->id());
	if (!user)
	{
		user = RoomManager::get_instance()->create_user(appid, roomid, uid, unit->conn->id());
	}
	user->set_role(rtc::RoleNull);
	roomptr->add_user(user);*/

	Json::Value ret_json;
	ret_json["action"] = "JoinResp";
	ret_json["ret_code"] = 0;
	send(ret_json, unit->conn);
}

void RtcLogicHandle::send(const Json::Value& json, std::shared_ptr<uvcore::TcpConnection> ptr)
{
	std::string bufs = json.toStyledString();
	std::string send_bufs;
	uvcore::PacketHelpers::pack(0, 100, bufs, send_bufs, 0);
	ptr->writeInLoop(send_bufs.c_str(), send_bufs.size());
}