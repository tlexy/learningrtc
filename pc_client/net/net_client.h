#ifndef LEARNING_RTC_NET_CLIENT_H
#define LEARNING_RTC_NET_CLIENT_H

#include <utility>
#include <uvnet/core/tcp_client.h>
#include <iostream>
#include <memory>
#include <uvnet/core/ip_address.h>
#include <common/util/sutil.h>
#include <common/def/rtc_common_def.h>

class NetTcpClient : public uvcore::TcpClient
{
public:
	NetTcpClient(std::shared_ptr<uvcore::EventLoop> loop, const uvcore::IpAddress& addr);

	virtual void on_message(std::shared_ptr<uvcore::TcpConnection> ptr);

	virtual void on_connection_close(std::shared_ptr<uvcore::TcpConnection> ptr);

	void connect();

	//业务操作
	bool join_room(const std::string& appid, const std::string& roomid, int64_t uid);
	bool leave_room(const  std::string& appid, const std::string& roomid, int64_t uid);

	bool publish_stream(const Json::Value& audio_desc, const Json::Value& video_desc);
	bool unpublish_stream(const Json::Value& audio_desc, const Json::Value& video_desc);

	bool subscribe_stream(int mask);
	bool unsubscribe_stream(int mask);

private:
	void on_connected(int status, std::shared_ptr<uvcore::TcpConnection> ptr);
	void send(const Json::Value&);
	void send_join_req();

private:
	bool _is_connected{ false };
	rtc::ClientStat _state{ rtc::InitStat };
	std::string _appid;
	std::string _roomid;
	int64_t _uid;
};

#endif