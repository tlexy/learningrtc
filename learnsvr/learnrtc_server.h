#ifndef LEARNING_RTC_SERVER_H
#define LEARNING_RTC_SERVER_H

#include <memory>

class RtcLogicHandle;
class IoServer;

class LearnRtcServer
{
public:
	void start(int port);

private:
	int _port;
	std::shared_ptr<IoServer> _io_server;
};

#endif