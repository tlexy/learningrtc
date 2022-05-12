#pragma once

#include <stdint.h>
#include <unordered_map>

/// <summary>
/// 音频设置初始化相关，只会检索双通道设备
/// </summary>
class AudioCommon
{
public:
	static void init_device();

public:
	static std::unordered_map<int, const PaDeviceInfo*> bgm_device_list;
	static std::unordered_map<int, const PaDeviceInfo*> mic_device_list;
	static std::unordered_map<int, const PaDeviceInfo*> speaker_device_list;
};

