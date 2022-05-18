#include "net_client.h"
#include <uvnet/core/tcp_server.h>
#include <uvnet/core/tcp_connection.h>
#include <uvnet/core/timer_event_loop.h>
#include <uvnet/core/timer.h>
#include <uvnet/core/event_loop.h>
#include "common/util/sutil.h"

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
		std::string hello("hello, uvnet");
		std::cout << "connected." << std::endl;
		ptr->write(hello.c_str(), hello.size());
	}
	else
	{
		std::cout << "connect failed" << std::endl;
	}
}