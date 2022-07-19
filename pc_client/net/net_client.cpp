#include "net_client.h"
#include <uvnet/core/tcp_server.h>
#include <uvnet/core/tcp_connection.h>
#include <uvnet/core/timer_event_loop.h>
#include <uvnet/core/timer.h>
#include <uvnet/core/event_loop.h>
#include <uvnet/core/packet_helpers.h>
#include "../common/pc_global.h"
#include "../component/comm_thread.h"
#include <log4u/core/common_log.h>

#define PROTO_HEADER_LEN 16

NetTcpClient::NetTcpClient(std::shared_ptr<uvcore::EventLoop> loop, const uvcore::IpAddress& addr)
	:TcpClient(loop, addr)
{}

void NetTcpClient::on_message(std::shared_ptr<uvcore::TcpConnection> ptr)
{
	while (true)
	{
		int payload_len = uvcore::PacketHelpers::unpacket_test(ptr->get_inner_buffer()->read_ptr(),
			ptr->get_inner_buffer()->readable_size());
		if (payload_len < 0 || payload_len > 16384 * 1024)
		{
			break;
		}
		memset(_buff, 0x0, sizeof(_buff));
		int ret = uvcore::PacketHelpers::unpack(_packet_header, (uint8_t*)_buff, sizeof(_buff),
			ptr->get_inner_buffer()->read_ptr(), ptr->get_inner_buffer()->readable_size());
		if (ret >= 0)
		{
			std::cout << "payload: " << _buff << std::endl;
			ptr->get_inner_buffer()->has_read(ret + PROTO_HEADER_LEN);
			handle_msg(_buff);
		}
		else
		{
			//loge("unpack error, payload_len: %d, error: %d", payload_len, ret);
		}
	}
	std::string recv_msg((char*)ptr->get_inner_buffer()->read_ptr(), ptr->get_inner_buffer()->readable_size());
	std::cout << "on message: " << recv_msg.c_str() << std::endl;

	Json::Value json;
	SUtil::parseJson("", json);

	ptr->get_inner_buffer()->has_read(recv_msg.size());
}

void NetTcpClient::handle_msg(const std::string& payload)
{
	Json::Value json;
	bool flag = SUtil::parseJson(payload.c_str(), json);
	if (!flag)
	{
		log_info("parse json data error");
		return;
	}

	std::string action = json["action"].asString();
	if (action == "JoinResp")
	{
		handle_join_resp(json);
	}
}

void NetTcpClient::handle_join_resp(const Json::Value& json)
{
	SignalHub sig;
	sig.first = 1;
	sig.t = std::make_any<int>(199);
	PcGlobal::get_instance()->comm_thread()->push(sig);
}

void NetTcpClient::on_connection_close(std::shared_ptr<uvcore::TcpConnection> ptr)
{}

void NetTcpClient::connect()
{
	using namespace std::placeholders;
	async_connect(std::bind(&NetTcpClient::on_connected, this, _1, _2));
}

void NetTcpClient::on_connected(int status, std::shared_ptr<uvcore::TcpConnection> ptr)
{
	if (status == 0)
	{
		if (_state == rtc::WaitJoin)
		{
			send_join_req();
		}
		_is_connected = true;
	}
	else
	{
		std::cout << "connect failed" << std::endl;
	}
	
}

bool NetTcpClient::join_room(const std::string& appid, const std::string& roomid, int64_t uid)
{
	_appid = appid;
	_roomid = roomid;
	_uid = uid;
	if (!_is_connected)
	{
		connect();
		_state = rtc::WaitJoin;
	}
	else
	{
		//发送join请求
		send_join_req();
	}
	return true;
}

void NetTcpClient::send_join_req()
{
	Json::Value json;
	json["action"] = "Join";
	json["appid"] = _appid;
	json["roomid"] = _roomid;
	json["uid"] = _uid;
	send(json);
	_state = rtc::WaitJoinResp;
}

bool NetTcpClient::leave_room(const  std::string& appid, const std::string& roomid, int64_t uid)
{
	return false;
}

bool NetTcpClient::publish_stream(const Json::Value& audio_desc, const Json::Value& video_desc)
{
	return false;
}

bool NetTcpClient::unpublish_stream(const Json::Value& audio_desc, const Json::Value& video_desc)
{
	return false;
}

bool NetTcpClient::subscribe_stream(int mask)
{
	return false;
}

bool NetTcpClient::unsubscribe_stream(int mask)
{
	return false;
}

void NetTcpClient::send(const Json::Value& json)
{
	std::string bufs = json.toStyledString();
	std::string send_bufs;
	uvcore::PacketHelpers::pack(0, 100, bufs, send_bufs, 0);
	_conn_ptr->writeInLoop(send_bufs.c_str(), send_bufs.size());
}