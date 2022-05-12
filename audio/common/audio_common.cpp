#include "audio_common.h"

void AudioCommon::init_device()
{
	PaError err = Pa_Initialize();
	if (err != paNoError)
	{
		return;
	}
	PaHostApiIndex hostCnt = Pa_GetHostApiCount();
	for (int i = 0; i < hostCnt; ++i)
	{
		const PaHostApiInfo* hinfo = Pa_GetHostApiInfo(i);
		if (hinfo->type == paWASAPI/* || hinfo->type == paDirectSound*/)
		{
			for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; ++hostDevice)
			{
				PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(i, hostDevice);
				const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(deviceNum);
				if (dinfo != NULL && dinfo->maxInputChannels > 0)
				{
					_bgm_device_list[deviceNum] = dinfo;
					_cb_bgm->addItem(QString::fromStdString(dinfo->name), QVariant(deviceNum));
				}
			}
		}
		else if (hinfo->type == paDirectSound)
		{
			for (PaDeviceIndex hostDevice = 0; hostDevice < hinfo->deviceCount; ++hostDevice)
			{
				PaDeviceIndex deviceNum = Pa_HostApiDeviceIndexToDeviceIndex(i, hostDevice);
				const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(deviceNum);
				if (dinfo != NULL && dinfo->maxInputChannels > 0)
				{
					_mic_device_list[deviceNum] = dinfo;
					_cb_mic->addItem(QString::fromStdString(dinfo->name), QVariant(deviceNum));
					//将设备保存到录音对象中
					PortRecorder::addPaDevice(deviceNum, dinfo);
				}
			}
		}
	}
}