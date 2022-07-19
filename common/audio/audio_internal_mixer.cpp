#include "audio_internal_mixer.h"

namespace wymediaServer{
	
	
inline bool IsOverFlow(int32_t sample)
{
	if (sample > 32767 || sample < -32768)
	{
		return true;
	}
	return false;
}

inline void SmoothAdjust(double last, double& target)
{
	if (target < last)
	{
		if (last > dFACTOR_STEP)
		{
			target = last - dFACTOR_STEP;
		}
	}
	else
	{
		if (last + dFACTOR_STEP < 1.0)
		{
			target = last + dFACTOR_STEP;
		}
	}
}
	


//CAudioBlockMixer
CAudioBlockMixer::CAudioBlockMixer(int sampleCount, int channels) : m_sampleCount(sampleCount), m_channels(channels), m_pBuffer(NULL)
{
	m_blockSize = sampleCount * channels * sizeof(int16_t);
}

CAudioBlockMixer::~CAudioBlockMixer()
{
	if (m_pBuffer != NULL)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
	}
}

void CAudioBlockMixer::Destroy()
{
	delete this;
}

bool CAudioBlockMixer::IsFormatChange(int sampleCount, int channel)
{
	return (m_sampleCount != sampleCount) || (m_channels != channel);
}

bool CAudioBlockMixer::Process(const AudioBlockList& inBlocks, std::string& outBlock)
{
	if (inBlocks.empty() || outBlock.size() != m_blockSize)
	{
		return false;
	}

	for (int i = 0; i < inBlocks.size(); i++)
	{
		if (inBlocks[i] == NULL)
		{
			continue;
		}
		if (inBlocks[i]->size() != m_blockSize)
		{
			return false;
		}
	}

	//outBlock.clear();
	if (inBlocks.size() == 1)
	{
		//outBlock.append(*inBlocks[0]);
		if (inBlocks[0] != NULL)
		{
			outBlock = *inBlocks[0];
		}
	}
	else
	{
		if (m_pBuffer == NULL)
		{
			m_pBuffer = (int32_t*)malloc(m_sampleCount * sizeof(int32_t) * m_channels);
		}
		MixToData(inBlocks, m_channels, m_sampleCount, m_pBuffer); 
		AdjustToBlock(m_pBuffer, outBlock);
	}
	return true;
}

void CAudioBlockMixer::MixToData(const AudioBlockList& inBlocks, uint32_t channels, uint32_t sampleCount, int32_t* pOut)
{
	int32_t mixTotal = 0;

	if (m_mixAdjs.size() != channels)
	{
		m_mixAdjs.clear();
		m_lastMixAdjs.clear();
		for (uint32_t i = 0; i < channels; i++)
		{
			m_mixAdjs.push_back(0.0);
			m_lastMixAdjs.push_back(1.0);
		}
	}

	for (uint32_t i = 0; i < channels; i++)
	{
		m_mixAdjs[i] = 1.0;

		for (uint32_t j = 0; j < sampleCount; j++)
		{
			mixTotal = 0;
			for (uint32_t k = 0; k < inBlocks.size(); k++)
			{
				if (inBlocks[k] == NULL)
				{
					continue;
				}
				//do mix
				int16_t *tmp = (int16_t*)(inBlocks[k]->c_str()) + (j * channels + i);
				
				mixTotal += *tmp;
			}

			*(pOut + j * channels + i) = mixTotal;

			if (IsOverFlow(mixTotal))
			{
				double tempAdj = 1.0;
				if (mixTotal > 0)
				{
					tempAdj = 32767.0 / mixTotal;
				}
				else
				{
					tempAdj = -32768.0 / mixTotal;
				}
				if (tempAdj < m_mixAdjs[i])
				{
					m_mixAdjs[i] = tempAdj;
				}
			}
		}
	}
}

void CAudioBlockMixer::AdjustToBlock(int32_t* pSrc, std::string& block)
{
	int32_t sample = 0.0;
	//block.resize(m_blockSize);
	int16_t * pOutData = (int16_t*)block.c_str();
	for (int i = 0; i < m_channels; i++)
	{
		SmoothAdjust(m_lastMixAdjs[i], m_mixAdjs[i]);
		m_lastMixAdjs[i] = m_mixAdjs[i];

		for (int j = 0; j < m_sampleCount; j++)
		{
			sample = int32_t(*(pSrc + j * m_channels + i) * m_mixAdjs[i]);
			CLAMP(sample, -32768, 32767);

			pOutData[j * m_channels + i] = (int16_t)sample;
		}
	}
}


} //namespace yyaudio


wymediaServer::IAudioBlockMixer* CreateAudioBlockMixer(int sampleCount, int channels)
{
	return new wymediaServer::CAudioBlockMixer(sampleCount, channels);
}
