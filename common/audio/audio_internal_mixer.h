#ifndef __AUDIO_INTERNAL_MIXER_H__
#define __AUDIO_INTERNAL_MIXER_H__

#include <string>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
//#include "../interface/yy_audio_mixer_i.h"
#include "mixer_interface.h"

namespace wymediaServer{
	
const double dFACTOR_STEP = 1.0 / 32;

#define CLAMP(t, tMin, tMax) \
if ((t) < (tMin)) (t) = (tMin); \
if ((t) > (tMax)) (t) = (tMax)

class CAudioBlockMixer : public IAudioBlockMixer
{
public:
	//sampleCount = 44100*(t ms/1000),10毫秒的数据的sampleCount为441
	CAudioBlockMixer(int sampleCount, int channels);
	~CAudioBlockMixer();

	void Destroy();
	bool Process(const AudioBlockList& inBlocks, std::string& outBlock);
	bool IsFormatChange(int sampleCount, int channel);

private:
	void MixToData(const AudioBlockList& inBlocks, uint32_t channels, uint32_t sampleCount, int32_t* pOut);
	void AdjustToBlock(int32_t* pSrc, std::string& block);

	int m_channels;
	std::vector<double> m_mixAdjs;
	std::vector<double> m_lastMixAdjs;
	int32_t* m_pBuffer;
	int m_sampleCount;
	int m_blockSize;
};

}//namespace wymediaServer

#endif //#ifndef __AUDIO_INTERNAL_MIXER_H__