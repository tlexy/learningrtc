#ifndef PORT_RECORDER_H
#define PORT_RECORDER_H

#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <memory>
#include <chrono>
#include <portaudio.h>

#include "../core/mid_buf.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
//#include <windows.h>

#pragma execution_character_set("utf-8")

#define NOISE_CANCEL

class IPortCallBack;
class NoiseSuppresion;

class PortRecorder
{
public:
	PortRecorder(IPortCallBack*);
	~PortRecorder();
	
	void set_target_sample_rate(int sample_rate);
	bool startRecord(int paDeviceIndex);
	void stopRecord();

	int doNsSwr(const char* buf, int frameCount, char* out_buf, int outlen);

	void thread_record(int index);

	static struct SwrContext* init_swr(int src_rate, int src_channels, int src_fmt, int dst_rate, int dst_channels, int dst_fmt);
	static int do_swr(struct SwrContext* swr, void* in_data, int nb_in_samples, int src_rate, void* out_data, int out_len, int dst_rate);
	static void addPaDevice(int, const PaDeviceInfo*);
public:
	struct SwrContext* _target_swr_ctx{ NULL };
	int _frame_size{1024};//录制时根据采样率肯定的一帧大小
	
	int _sample_size;
	bool _is_record_stop;
	int _error_count;
	struct SwrContext* _swr_ctx;
	IPortCallBack* _cb;
	int _src_rate{ 0 };
	AVSampleFormat _src_fmt{ AV_SAMPLE_FMT_S16 };
	int _dst_rate{ 44100 };
	int _recorder_id;

	static std::map<int, const PaDeviceInfo*> device_list;
private:
	PaStream* _stream{NULL};//正在录制的流handle
	int _target_sample_rate{0};
	std::shared_ptr<std::thread> _record_thread{nullptr};
	static int _recorder_id_s;

};

#endif