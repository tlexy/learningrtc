#ifndef _MIXER_INTERFACE_H_
#define _MIXER_INTERFACE_H_

#include <string>
#include <vector>

namespace wymediaServer{
	
	
typedef std::vector<std::string*> AudioBlockList;
//only for the same samplerate and channels
class IAudioBlockMixer
{
public:
	virtual ~IAudioBlockMixer(){}
	virtual void Destroy() = 0;
	virtual bool Process(const AudioBlockList& inBlocks, std::string& outBlock) = 0;
	virtual bool IsFormatChange(int sampleCount, int channel) = 0;
};
	
	
} //namespace wymediaServer	

wymediaServer::IAudioBlockMixer* CreateAudioBlockMixer(int sampleCount, int channels);  // the sampleCount and channels should be the same in all AudioBlocks.

#endif //#ifndef _MIXER_INTERFACE_H_