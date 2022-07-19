#ifndef LXP_AUDIO_PLAYER_H
#define LXP_AUDIO_PLAYER_H

#include <portaudio.h>
#include <stdint.h>
#include <unordered_map>
#include <functional>
#include "../common/audio_buffer.h"
#include <mutex>

#pragma execution_character_set("utf-8")

using AudioPlayerCallBack = std::function<void(void* output, unsigned long frameCount)>;

/// <summary>
/// 主动填充数据的版本
/// </summary>
class AudioPlayer
{
public:
	AudioPlayer();

	static void add_pa_device(int deviceIdx, const PaDeviceInfo* dinfo);

	//只有返回0才代表初始化成功
	int init_play(int device_idx);
	int init_play();

	void set_player_cb(AudioPlayerCallBack cb);

	int play();
	void stop();
	void pause();
	//外部填充数据
	void fill_data(const uint8_t*, int len);

	//内部播放数据，当播放器需要数据时，会调用这个函数以获取数据
	virtual void read_data(void* dst, int frameCount);

	uint32_t format();
	int sample_rate();
	void set_sample_rate(int sample_rate);
	int channel_count();

	//获取设备id及设备名称

private:
	//设备列表
	static std::unordered_map<int, const PaDeviceInfo*> _device_list;
	PaStream* _stream;
	bool _is_stop{ true };
	AudioPlayerCallBack _cb{nullptr};
	int _sample_rate;
	uint32_t _format;
	int _deps;
	int _channel_count{2};
	//音频数据缓冲
	AudioBuffer* _play_buff;
	std::mutex _play_buff_mutex;
};

#endif