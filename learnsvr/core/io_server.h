#ifndef LEARNING_RTC_IO_SERVER_H
#define LEARNING_RTC_IO_SERVER_H

#include <uvnet/server/general_server.h>
#include <unordered_map>
#include <string>
#include <uvnet/core/packet_helpers.h>

namespace uvcore {
	class Timer;
	class TcpConnection;
}

class IoServer : public uvcore::GeneralServer
{
public:
	IoServer();

protected:
	virtual void on_newconnection(std::shared_ptr<uvcore::TcpConnection>);
	virtual void on_message(std::shared_ptr<uvcore::TcpConnection>);
	virtual void on_connection_close(std::shared_ptr<uvcore::TcpConnection>);

	virtual void timer_event(uvcore::Timer*);

private:
	uvcore::packet_t _packet_header;
	std::string _payload;
	std::unordered_map<int64_t, std::shared_ptr<uvcore::TcpConnection>> _conn_map;

	char _buff[1024 * 1024];
};

#endif