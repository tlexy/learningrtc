#include "learnrtc_server.h"
#include "core/logic_handle.h"
#include "core/io_server.h"

void LearnRtcServer::start(int port)
{
	_port = port;

	_io_server = std::make_shared<IoServer>();
	_io_server->async_io_start("0.0.0.0", port);

	RtcLogicHandle::get_instance()->init();
	RtcLogicHandle::get_instance()->start_thread();

}