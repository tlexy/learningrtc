#ifndef LXP_COMM_THREAD_H
#define LXP_COMM_THREAD_H

#include <QThread>
#include <common/util/threadqueue.hpp>
#include <memory>
#include <unordered_map>
#include <QSize>
#include <functional>
#include "../common/pc_common_def.h"

using SignalFunc = std::function<void(const std::any&)>;

//UI与逻辑通讯线程
class CommThread : public QThread
{
	Q_OBJECT
public:
	CommThread(size_t max_queue = 100);
	void init();
	void push(const SignalHub& sig);
	void stop();

signals:
	int sig_join_resp(int);

protected:
	void run();

private:
	void do_sig_join_resp(const std::any&);


private:
	ThreadQueue<SignalHub> _fr_queue;
	bool _is_stop{ false };
	std::unordered_map<int, SignalFunc> _signal_hubs;
};

#endif