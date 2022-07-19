#ifndef LEARNING_RTC_PC_GLOBAL_H
#define LEARNING_RTC_PC_GLOBAL_H

#include <memory>

class CommThread;

class PcGlobal
{
public:
	static PcGlobal* get_instance();

	void init();
	void destroy();

	std::shared_ptr<CommThread> comm_thread();

private:
	static PcGlobal* const _instance;

	std::shared_ptr<CommThread> _comm_thread;
};

#endif