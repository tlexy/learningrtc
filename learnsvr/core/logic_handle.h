#ifndef LEARNING_RTC_IO_SERVER_LOGIC_H
#define LEARNING_RTC_IO_SERVER_LOGIC_H

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <jsoncpp/json/json.h>
#include <list>
#include <thread>
#include <mutex>

namespace uvcore {
	class TcpConnection;
}

class MsgUnit
{
public:
	std::string action;
	std::shared_ptr<Json::Value> json;
	std::shared_ptr<uvcore::TcpConnection> conn;
};

using RtcHandlFunc = std::function<void(std::shared_ptr<MsgUnit>)>;

class RtcLogicHandle
{
public:
	RtcLogicHandle();
	static RtcLogicHandle* get_instance();
	void init();

	void handle_msg(const std::string& buff, std::shared_ptr<uvcore::TcpConnection> ptr);

	void start_thread();
	void stop_thread();

private:
	void update();
	void send(const Json::Value& json, std::shared_ptr<uvcore::TcpConnection> ptr);
	//业务逻辑
	void handle_join(std::shared_ptr<MsgUnit> unit);


private:
	static RtcLogicHandle* const _instance;
	std::unordered_map<std::string, RtcHandlFunc> _func_map;

	bool _is_thread_stop{ true };
	std::shared_ptr<std::thread> _th{nullptr};
	std::mutex _queue_mutex;
	std::list<std::shared_ptr<MsgUnit>> _msg_queue;

};

#endif