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
class H264FFmpegDecoder;

class PcmBuffer
{
public:
	uint8_t* data = nullptr;
	uint32_t max_size = 4096;
	uint32_t write_pos = 0;
	uint32_t read_pos = 0;
	uint32_t timestamp = 0;
};

class StreamsJitterBufferEntity : public JitterBufferEntity
{
public:
	StreamsJitterBufferEntity();

	void init() override;
	//UDP接收到一个rtp包
	void push(rtp_packet_t*) override;
	//void update() override;

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

	void get_video_info(int& width, int& height, int& fps);

protected:
	virtual bool force_cache();
	virtual void do_decode();

	//从cache中收到rtp包
	void on_rtp_packet(rtp_packet_t*) override;

private:
	void aac_init();
	void do_decode_aac();
	void do_decode_h264();

	int64_t get_video_pts(int64_t audio_pts);

private:
	std::shared_ptr<RtpCacher> _h264_cacher;
	std::shared_ptr<RtpH264Decoder> _rtp_h264_decoder;
	std::shared_ptr<H264FFmpegDecoder> _h264_decoder;
	std::list<NALU*> _nalus;
	std::mutex _nalus_mutex;
	std::list<AVFrame*> _frames;
	int _vw = 0;//视频的宽
	int _vh = 0;//视频的高
	int _fps = 0;

	//aac解码相关
	std::shared_ptr<AacHelper> _aac_helper{ nullptr };
	int _bit_dep{ 0 };
	int _channel{ 0 };
	int _sample_rate{ 0 };
	bool _output_init{ false };
	int _frame_size;
	int64_t _output_len{ 0 };//
	uint8_t* _decode_buf;
	int _decode_buf_len;
	std::list<std::shared_ptr<PcmBuffer>> _pcm_buffers;
	int _pcm_buffer_count = 0;
	std::mutex _pcm_buffer_mutex;
};

#endif