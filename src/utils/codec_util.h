#pragma once
#include "decode_audio/wavhead.h"

namespace bigoai {

std::string get_wav_audio(ContextPtr ctx) {
    const auto &pcm = ctx->pcm_audio();
    const auto &sr = ctx->pcm_sample_rate();
    if (pcm.size() == 0 || sr == 0)
        return "";
    WavFileHead wav_header;
    WavFileHead_Init(&wav_header, 1, sr, 16);
    int all_len = pcm.size() + sizeof(wav_header);
    WavFileHead_SetFileLen(&wav_header, all_len);
    std::string wav((char *)&wav_header, sizeof(wav_header));
    return wav + pcm;
}

}