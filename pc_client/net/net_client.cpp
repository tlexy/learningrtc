#include "net_client.h"
#include <uvnet/core/tcp_server.h>
#include <uvnet/core/tcp_connection.h>
#include <uvnet/core/timer_event_loop.h>
#include <uvnet/core/timer.h>
#include <uvnet/core/event_loop.h>
#include <uvnet/core/packet_helpers.h>

NetTcpClient::NetTcpClient(std::shared_ptr<uvcore::EventLoop> loop, const uvcore::IpAddress& addr)
	:TcpClient(loop, addr)
{}

void NetTcpClient::on_message(std::shared_ptr<uvcore::TcpConnection> ptr)
{
	std::string recv_msg((char*)ptr->get_inner_buffer()->read_ptr(), ptr->get_inner_buffer()->readable_size());
	std::cout << "on message: " << recv_msg.c_str() << std::endl;

	Json::Value json;
	SUtil::parseJson("", json);

	ptr->get_inner_buffer()->has_read(recv_msg.size());
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
		/*std::string hello("hello, uvnet");
		std::cout << "connected." << std::endl;
		ptr->write(hello.c_str(), hello.size());*/
		if (_state == rtc::WaitJoin)
		{
			send_join_req();
		}
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
{}

bool NetTcpClient::publish_stream(const Json::Value& audio_desc, const Json::Value& video_desc)
{}

bool NetTcpClient::unpublish_stream(const Json::Value& audio_desc, const Json::Value& video_desc)
{}

bool NetTcpClient::subscribe_stream(int mask)
{}

bool NetTcpClient::unsubscribe_stream(int mask)
{}

void NetTcpClient::send(const Json::Value& json)
{
	std::string bufs = json.toStyledString();
	std::string send_bufs;
	uvcore::PacketHelpers::pack(0, 100, bufs, send_bufs, 0);
	_conn_ptr->writeInLoop(send_bufs.c_str(), send_bufs.size());
}