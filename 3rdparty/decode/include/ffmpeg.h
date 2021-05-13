/*!
 * Copyright (c) 2017 by Contributors
 * \file ffmpeg.hpp
 * \author liangzhujin
 * \brief 
*/

#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

#include "util.h"

struct DecodeParam {
  /*! \brief input filename */
  const char *fname = nullptr;
  const uint8_t *video_data = nullptr;
  size_t video_size = 0;
  /*! \brief output image data per second or per frame */
  bool second_mode = false;
  /*! \brief only decode top n frame */
  int top_n = -1;

  bool only_audio = false;

  int req_sample_rate = 22050;
  int req_channels    = 1; 
};

class FFMpegVideoDecoder {
 public:
  /*!
   * \brief construction function
   * \return 
   */
  FFMpegVideoDecoder();

  /*!
   * \brief Init decoder.
   * \param 
   */
  int Init(int dev_id = -1, int use_nvidia_sdk = 0, int height = 0, int width = 0);

  /*!
   * \brief decode a video specified by file name
   * \param fname input filename
   * \param second_mode output image data per second or per frame
   * \param im_data output image data
   * \param im_info output image info
   * \return
   */
  int DecodeVideo(const char *fname, 
                  const bool second_mode, 
                  std::vector<uint8_t> *video_data, 
                  std::vector<float> *audio_data,
                  VideoInfo *video_info);
  /*!
   * \brief decode a video specified by decode parameters
   * \param decode_param_p pointer of decode parameters
   * \param im_data output image data
   * \param im_info output image info
   * \return
   */
  int DecodeVideo(DecodeParam *decode_param_p, 
                  std::vector<uint8_t> *video_data, 
                  std::vector<float> *audio_data,
                  VideoInfo *video_info);
  /*!
   * \brief decode a video from memory data
   * \param video_data input video data
   * \param video_size input video data size
   * \param second_mode output image data per second or per frame
   * \param im_data output image data
   * \param im_info output image info
   * \return
   */
  int DecodeVideo(const uint8_t *input_data, 
                  const size_t input_size, 
                  const bool second_mode,
                  std::vector<uint8_t> *video_data, 
                  std::vector<float> *audio_data,
                  VideoInfo *video_info,
                  int sample_rate = 22050, 
                  int req_channels = 1);

  int DecodeAudio(const uint8_t *input_data,
                  const size_t input_size,
                  const int sample_rate,
                  const int req_channels,
                  std::vector<float> *audio_data);

private:
  int dev_id_ = -1;
  int height_ = 0;
  int width_  = 0;
  int use_naive_cuvid_ = 0;
  int is_init_ = 0;
};
