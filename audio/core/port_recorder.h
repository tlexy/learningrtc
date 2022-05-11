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
#include <windows.h>

#pragma execution_character_set("utf-8")

#define NOISE_CANCEL

class IPortCallBack;
class NoiseSuppresion;

class PortRecorder
{
public:
	PortRecorder(IPortCallBack*);
	~PortRecorder();
	
	bool startRecord(int paDeviceIndex);
	void stopRecord();

	void onChangeANC();
	inline void ChangeANC();
	void startANC(int mode);
	bool needANC();

	int doNsSwr(const char* buf, int frameCount, char* out_buf, int outlen);

	void thread_record(int index);

	static struct SwrContext* init_swr(int src_rate, int src_channels, int src_fmt, int dst_rate, int dst_channels, int dst_fmt);
	static int do_swr(struct SwrContext* swr, void* in_data, int nb_in_samples, int src_rate, void* out_data, int out_len, int dst_rate);
	static void addPaDevice(int, const PaDeviceInfo*);
public:
	NoiseSuppresion* _ns{NULL};//ANC
	bool _is_anc_start{false};
	struct SwrContext* _ns_swr_ctx{ NULL };
	struct SwrContext* _target_swr_ctx{ NULL };
	char* _ns_swr_buf{ NULL };//4K > 960*2*2
	int _ns_swr_buf_size{0};

	//char* _ns_prebuf{ NULL };//降噪前的buf 8K
	//int _ns_prebuf_size{0};
	//int _ns_prebuf_pos{0};
	//char* _ns_temp_buf{ NULL };//1k
	mid_buf* _ns_pre_mid{ NULL };

	char* _ns_afterbuf{ NULL };//降噪后的buf 1M
	int _ns_after_size{0};
	int _ns_after_pos{0};
	int _ns_after_read_pos{0};
	int _ns_cache_limit{3};
	int _ns_cache_count{0};
	char* _ns_afterbuf_temp{ NULL }; //10k
	int _ns_afterbuf_temp_len{ 0 };

	static std::map<int, const PaDeviceInfo*> device_list;

	int _frame_size{1024};//录制时根据采样率肯定的一帧大小
	int _ns_frame_size{743};//噪声消除后重采样的的一帧大小
	int _sample_size;
	bool _is_record_stop;
	int _error_count;
	struct SwrContext* _swr_ctx;
	IPortCallBack* _cb;
	int _src_rate{ 0 };
	AVSampleFormat _src_fmt{ AV_SAMPLE_FMT_S16 };
	int _dst_rate{ 44100 };
	int _ns_level{0};
	int _ns_step{ 1280 };
	bool _need_update_ns{true};
	int _recorder_id;
private:
	PaStream* _stream{NULL};//正在录制的流handle
	std::shared_ptr<std::thread> _record_thread;
	std::vector<NoiseSuppresion*> _level_anc;
	static int _recorder_id_s;

};

#endif