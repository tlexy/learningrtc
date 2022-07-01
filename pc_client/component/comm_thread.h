﻿#ifndef LXP_COMM_THREAD_H
#define LXP_COMM_THREAD_H

#include <QThread>
#include <common/util/threadqueue.hpp>
#include <memory>
#include <unordered_map>
#include <QSize>
#include <functional>
#include "../common/pc_common_def.h"

using SignalFunc = std::function<void(const std::any&)>;

//Q_DECLARE_METATYPE(VideoParameter);

#define PUSH_SIG(signo, par, T) SignalHub sig; \
	sig.first = signo; \
	sig.t = std::make_any<T>(par); \
	PcGlobal::get_instance()->comm_thread()->push(sig)

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
	void sig_join_resp(int);
	void sig_video_ready(const VideoParameter&);
	void sig_video_frame(const VideoFrame&);
	

protected:
	void run();

private:
	void do_sig_join_resp(const std::any&);
	void do_sig_video_ready(const std::any&);
	void do_sig_video_frame(const std::any&);

private:
	ThreadQueue<SignalHub> _fr_queue;
	bool _is_stop{ false };
	std::unordered_map<int, SignalFunc> _signal_hubs;
};

#endif