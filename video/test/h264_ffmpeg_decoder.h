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

private:
	AVCodec* _ctx{nullptr};
	AVCodecContext* _ctx = nullptr;
	struct AVPacket _av_packet;
	struct AVFrame* _av_frame = nullptr;
	struct AVFrame* _yuv_frame = nullptr;
};

#endif