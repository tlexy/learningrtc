#ifndef LEARNING_RTC_NET_CLIENT_H
#define LEARNING_RTC_NET_CLIENT_H

#include <utility>
#include <uvnet/core/tcp_client.h>
#include <iostream>
#include <memory>
#include <uvnet/core/ip_address.h>

class NetTcpClient : public uvcore::TcpClient
{
public:
	NetTcpClient(std::shared_ptr<uvcore::EventLoop> loop, const uvcore::IpAddress& addr);

	virtual void on_message(std::shared_ptr<uvcore::TcpConnection> ptr);

	virtual void on_connection_close(std::shared_ptr<uvcore::TcpConnection> ptr);

	void connect();

private:
	void on_connected(int status, std::shared_ptr<uvcore::TcpConnection> ptr);
};

#endif