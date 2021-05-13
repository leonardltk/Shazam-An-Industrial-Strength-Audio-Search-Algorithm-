#pragma once
extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <vector>

int DecodeAudio(std::vector<AVPacket> &pkts, 
                int req_channels,
                int req_sample_rate,
                AVCodecContext *audio_codec_ctx, 
                std::vector<float> *audio_out);