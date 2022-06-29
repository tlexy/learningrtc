#ifndef H264_FFMPEG_DECODER_H
#define H264_FFMPEG_DECODER_H

#include <video/rtp_h264/rtp_h264_def.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class H264FFmpegDecoder
{
public:
	bool init();
	void decode(NALU* nalu);

	/// <summary>
	/// 返回大于等于0时，可以继续调用，小于0时无可用帧或者出错了
	/// </summary>
	/// <param name="out_frame"></param>
	/// <returns></returns>
	int receive_frame(struct AVFrame*& out_frame);

private:
	AVCodec* _av_codec{nullptr};
	AVCodecContext* _ctx = nullptr;
	struct AVPacket* _av_packet;
	struct AVFrame* _av_frame = nullptr;
	struct AVFrame* _yuv_frame = nullptr;
	int _ret;
};

#endif