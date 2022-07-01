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

    _av_packet = av_packet_alloc();
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
    _av_packet->data = nalu->payload;
    _av_packet->size = nalu->payload_len;
    _av_packet->pts = nalu->timestamp;

    _ret = avcodec_send_packet(_ctx, _av_packet);
   
}

int H264FFmpegDecoder::receive_frame(struct AVFrame*& out_frame)
{
    if (_ret >= 0)
    {
        AVFrame* frame = av_frame_alloc();
        _ret = avcodec_receive_frame(_ctx, frame);
        if (_ret == AVERROR_EOF)
        {
            av_frame_free(&frame);
            return -1;
        }
        else if (_ret == AVERROR(EAGAIN))
        {
            av_frame_free(&frame);
            return 0;
        }
       /* else if (_ret == AVERROR_INPUT_CHANGED)
        {
            av_frame_free(&frame);
            fprintf(stderr, "Error avcodec_receive_frame, ret: %d\n", _ret);
            return 0;
        }*/
        else if (_ret == AVERROR(EINVAL))
        {
            av_frame_free(&frame);
            fprintf(stderr, "Error avcodec_receive_frame, ret: %d\n", _ret);
            return -2;
        }
        else
        {
            out_frame = frame;
        }
        return 1;
    }
    return 0;
}