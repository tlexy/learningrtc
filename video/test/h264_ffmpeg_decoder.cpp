#include "h264_ffmpeg_decoder.h"
#include <iostream>

bool H264FFmpegDecoder::init()
{
    _av_codec = const_cast<AVCodec*>(avcodec_find_decoder(AV_CODEC_ID_H264));
    if (!_av_codec) {
        std::cout << "can not find H264 codec" << std::endl;
        return false;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(_av_codec);
    if (codec_ctx == NULL) {
        std::cout << "Could not alloc video context!" << std::endl;
        return false;
    }
    _ctx = codec_ctx;

    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(codec_ctx, _av_codec, NULL) < 0)
    {
        std::cout << "Could not open codec" << std::endl;
        return false;
    }

    av_init_packet(&_av_packet);
    _av_frame = av_frame_alloc();
    _yuv_frame = av_frame_alloc();
    return true;

    /*AVCodecParameters* codec_param = avcodec_parameters_alloc();
    if (avcodec_parameters_from_context(codec_param, codec_ctx) < 0) {
        std::cout << "Failed to copy avcodec parameters from codec context." << std::endl;
        avcodec_parameters_free(&codec_param);
        avcodec_free_context(&codec_ctx);
        return -3;
    }*/
}

void H264FFmpegDecoder::decode(NALU* nalu)
{
    _av_packet.data = nalu->payload;
    _av_packet.size = nalu->payload_len;

    int ret = avcodec_send_packet(_ctx, &_av_packet);
    while (ret >= 0)
    {
        ret = avcodec_receive_frame(_ctx, _av_frame);
        if (ret == AVERROR_EOF)
        {
            return;
        }
        else if (ret == AVERROR(EAGAIN))
        {
            break;
        }
        else if (ret < 0)
        {
            fprintf(stderr, "Error avcodec_receive_frame, ret: %d\n", ret);
            return;
        }
    }
}