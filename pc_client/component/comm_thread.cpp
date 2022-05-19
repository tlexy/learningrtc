#include "comm_thread.h"
#include <QDebug>

#define REG_SIG(TYPE, FUNC) _signal_hubs[TYPE] = std::bind(&CommThread::FUNC, this, _1);

CommThread::CommThread(size_t max_queue)
	:_fr_queue(max_queue)
{}

void CommThread::init()
{
	using namespace std::placeholders;

	_signal_hubs[eSigJoinResp] = std::bind(&CommThread::do_sig_join_resp, this, _1);
}

void CommThread::do_sig_join_resp(const std::any& sig)
{
	emit sig_join_resp(std::any_cast<int>(sig));
}

void CommThread::push(const SignalHub& sig)
{
	_fr_queue.push_back(sig);
}

void CommThread::stop()
{
	_is_stop = true;
}

void CommThread::run()
{
	bool flag = false;
	auto it = _signal_hubs.begin();
	while (!_is_stop)
	{
		auto sig = _fr_queue.pop(flag, std::chrono::milliseconds(10));
		if (!flag)
		{
			continue;
		}
		it = _signal_hubs.find(sig.first);
		if (it != _signal_hubs.end())
		{
			it->second(sig.t);
		}
	}
	qDebug() << "CommThread::run() break.";
}