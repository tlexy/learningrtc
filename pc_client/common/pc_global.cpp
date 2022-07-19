#include "pc_global.h"
#include "../component/comm_thread.h"
#include <thread>

PcGlobal* const PcGlobal::_instance = new PcGlobal();

PcGlobal* PcGlobal::get_instance()
{
	return _instance;
}

void PcGlobal::init()
{
	_comm_thread = std::make_shared<CommThread>();
	_comm_thread->init();
	_comm_thread->start();
}

void PcGlobal::destroy()
{
	_comm_thread->stop();
	_comm_thread->exit(0);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

std::shared_ptr<CommThread> PcGlobal::comm_thread()
{
	return _comm_thread;
}