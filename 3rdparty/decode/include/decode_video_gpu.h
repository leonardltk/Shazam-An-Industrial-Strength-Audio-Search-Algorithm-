#pragma once
extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <vector>
#include "util.h"

int DecodeGPU(const std::vector<AVPacket> &data,
              const VideoInfo& info,
              const std::vector<size_t >& decode_sequence,
              const std::vector<size_t >& frame_ind,
              std::vector<uint8_t> *out,
              int dev_id);

int DecodeGPU(const std::vector<std::vector<uint8_t>> &data,
              const VideoInfo& info,
              const std::vector<size_t >& decode_sequence,
              const std::vector<size_t >& frame_ind,
              std::vector<uint8_t> *out,
              int dev_id);