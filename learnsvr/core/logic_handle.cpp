#include "logic_handle.h"
#include <common/util/sutil.h>
#include <log4u/core/common_log.h>

RtcLogicHandle* const RtcLogicHandle::_instance = new RtcLogicHandle();

RtcLogicHandle::RtcLogicHandle()
{}

RtcLogicHandle* RtcLogicHandle::get_instance()
{
	return _instance;
}

void RtcLogicHandle::init()
{}

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
	while (_is_thread_stop)
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
{}