﻿#ifndef PORT_RECORDER_H
#define PORT_RECORDER_H

#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <memory>
#include <chrono>
#include <portaudio.h>

#include "../common/mid_buf.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#include <windows.h>

#pragma execution_character_set("utf-8")


class IPortCallBack;

class PortRecorder
{
public:
	PortRecorder(IPortCallBack*);
	~PortRecorder();
	
	void set_target_rate(int sample_rate);
	bool startRecord(int paDeviceIndex);
	void stopRecord();

	void thread_record(int index);

	static struct SwrContext* init_swr(int src_rate, int src_channels, int src_fmt, int dst_rate, int dst_channels, int dst_fmt);
	static int do_swr(struct SwrContext* swr, void* in_data, int nb_in_samples, int src_rate, void* out_data, int out_len, int dst_rate);
	static void addPaDevice(int, const PaDeviceInfo*);
public:
	mid_buf* record_mid{NULL};
	static std::map<int, const PaDeviceInfo*> device_list;

	int ten_mill_sample{ 441 };//10ms的样本大小
	int tem_mill_size{ 441 * 2 * 2 };
	int channel_size{ 2 };
	int sample_size;
	bool is_record_stop;
	int error_count;
	struct SwrContext* swr_ctx;
	IPortCallBack* cb{nullptr};
	int src_rate{ 0 };
	AVSampleFormat src_fmt{ AV_SAMPLE_FMT_S16 };
	int dst_rate{ 44100 };
private:
	PaStream* _stream{NULL};//正在录制的流handle
	std::shared_ptr<std::thread> _record_thread;

};

#endif