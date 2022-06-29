#ifndef LEARNING_RTC_STREAMS_JB_ENTITY_H
#define LEARNING_RTC_STREAMS_JB_ENTITY_H

#include <qos/entity/jitter_buffer_entity.h>
#include <video/rtp_h264/rtp_h264_decoder.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class RtpCacher;
class AacHelper;
class FileSaver;


class StreamsJitterBufferEntity : public JitterBufferEntity
{
public:
	StreamsJitterBufferEntity();

	void init() override;
	//UDP接收到一个rtp包
	void push(rtp_packet_t*) override;
	void update() override;
	//从cache中收到rtp包
	void on_rtp_packet(rtp_packet_t*) override;

	/// <summary>
	/// 将长度为len的PCM数据写入到data中，并带回audio的pts
	/// </summary>
	/// <param name="data"></param>
	/// <param name="len"></param>
	/// <param name="audio_pts">out pts</param>
	/// <returns></returns>
	int get_pcm_buffer(int8_t* data, int len, int64_t& audio_pts);

	/// <summary>
	/// 根据音频pts获取视频帧
	/// </summary>
	/// <param name=""></param>
	/// <param name="audio_pts"></param>
	/// <returns></returns>
	int get_video_frame(AVFrame*, int64_t audio_pts);

protected:
	virtual bool force_cache();
	virtual void do_decode();

private:
	std::shared_ptr<RtpCacher> _aac_cacher;
	std::shared_ptr<RtpCacher> _h264_cacher;
	std::shared_ptr<RtpH264Decoder> _rtp_h264_decoder;
};

#endif