#include <iostream>
#include <portaudio.h>
#include <stdio.h>
#include <unordered_map>

std::unordered_map<int, const PaDeviceInfo*> mic_device_list;

void printf_device_info(int index_num, const PaDeviceInfo* dinfo)
{
	if (!dinfo)
	{
		return;
	}
	printf("PaAudio info:: device_index: %d, name: %s, hostAPi: %d, maxInputChannels: %d, maxOutputChannels: %d, defaultLowInputLatency: %f, defaultLowOutputLatency: %f, defaultHighInputLatency: %f, defaultHighOutputLatency: %f, defaultSampleRate: %f\n",
		index_num, dinfo->name, dinfo->hostApi, dinfo->maxInputChannels, dinfo->maxOutputChannels, dinfo->defaultLowInputLatency, dinfo->defaultLowOutputLatency, dinfo->defaultHighInputLatency, dinfo->defaultHighOutputLatency, dinfo->defaultSampleRate);
}

void pa_init()
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
			//printf_device_info(deviceNum, dinfo);
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
			}
		}
	}
}

void destory()
{
	Pa_Terminate();
}

int port_record_cb(
	const void* input, void* output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	std::cout << "frame count: " << frameCount << std::endl;
	return paContinue;
}

void thread_record(int index)
{
	const PaDeviceInfo* dinfo = Pa_GetDeviceInfo(index);
	if (!dinfo)
	{
		printf("Pa_GetDeviceInfo return NULL");
		return;
	}

	PaStream* inputStream;
	PaStreamParameters inputParam;
	inputParam.channelCount = 2;
	inputParam.device = index;
	inputParam.sampleFormat = paFloat32;

	inputParam.hostApiSpecificStreamInfo = NULL;
	inputParam.suggestedLatency = dinfo->defaultLowInputLatency;//dinfo->defaultLowInputLatency;

	PaError err = Pa_IsFormatSupported(&inputParam, NULL, dinfo->defaultSampleRate);
	err = Pa_OpenStream(&inputStream, &inputParam, NULL, dinfo->defaultSampleRate, paFramesPerBufferUnspecified, 0, port_record_cb, NULL);

	err = Pa_StartStream(inputStream);
	if (err != paNoError)
	{
		printf("pa start stream error");
		return;
	}
}

bool start_record(int paDeviceIndex)
{
	if (mic_device_list.find(paDeviceIndex) == mic_device_list.end())
	{
		return false;
	}
	thread_record(paDeviceIndex);
	return true;
}

int main()
{
	pa_init();

	start_record(5);

	std::cin.get();
	destory();
	
	return 0;
}