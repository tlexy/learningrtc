#include "audio_common.h"
#include "../core/audio_player.h"
#include "../core/port_recorder.h"
#include <iostream>

#pragma execution_character_set("utf-8")

std::unordered_map<int, const PaDeviceInfo*> AudioCommon::bgm_device_list;
std::unordered_map<int, const PaDeviceInfo*> AudioCommon::mic_device_list;
std::unordered_map<int, const PaDeviceInfo*> AudioCommon::speaker_device_list;

void AudioCommon::init_device()
{
	//PaError err = Pa_Initialize();
	//if (err != paNoError)
	//{
	//	return;
	//}
	//PaHostApiIndex hostCnt = Pa_GetHostApiCount();
	//for (int i = 0; i < hostCnt; ++i)
	//{
	//	const PaHostApiInfo* hinfo = Pa_GetHostApiInfo(i);
	//	if (hinfo->type == paWASAPI/* || hinfo->type == paDirectSound*/)
	//	{
	//		for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; ++hostDevice)
	//		{
	//			PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(i, hostDevice);
	//			const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(deviceNum);
	//			if (dinfo != NULL && dinfo->maxInputChannels > 0)
	//			{
	//				std::cout << "paWASAPI index: " << deviceNum << "\tname:" << dinfo->name << std::endl;
	//			}
	//		}
	//	}
	//	else if (hinfo->type == paDirectSound)
	//	{
	//		for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; ++hostDevice)
	//		{
	//			PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(i, hostDevice);
	//			const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(deviceNum);
	//			if (dinfo != NULL && dinfo->maxInputChannels > 0)
	//			{
	//				//将设备保存到录音对象中
	//				PortRecorder::add_pa_device(deviceNum, dinfo);
	//				std::cout << "paDirectSound index: " << deviceNum << "\tname:" << dinfo->name << std::endl;
	//			}
	//		}
	//	}
	//}

	PaError err = Pa_Initialize();
	if (err != paNoError)
	{
		return;
	}
	PaHostApiIndex hostCnt = Pa_GetHostApiCount();
	for (int i = 0; i < hostCnt; ++i)
	{
		const PaHostApiInfo* hinfo = Pa_GetHostApiInfo(i);
		if (hinfo->type == paMME)
		{
			continue;
		}
		for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; ++hostDevice)
		{
			PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(i, hostDevice);
			const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(deviceNum);
			
			if (dinfo == NULL)
			{
				continue;
			}
			
			if (dinfo->maxInputChannels > 0)
			{
				std::cout << "input device, deviceNum: " << deviceNum << "\tname: " << dinfo->name << std::endl;
				if (hinfo->type == paWASAPI || hinfo->type == paDirectSound)
				{
					mic_device_list[deviceNum] = dinfo;
				}
				else
				{
					continue;
				}
				//将设备保存到录音对象中
				PortRecorder::add_pa_device(deviceNum, dinfo);
			}
			if (dinfo->maxOutputChannels > 0)
			{
				speaker_device_list[deviceNum] = dinfo;
				AudioPlayer::add_pa_device(deviceNum, dinfo);
			}
		}
	}
}

void AudioCommon::destory()
{
	Pa_Terminate();
}
