/*!
 * Copyright (c) 2018 by Contributors
 * \file webp.hpp
 * \author zhuangrongbin
 * \brief 
*/

#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "util.h"

struct DecodeWebPParam{
  /*! \brief input filename */
  const char* fname = nullptr;
  /*! \brief data to decode */
  const uint8_t *img_data = nullptr;
  /*! \brief size of data to decode */
  size_t data_size = 0;
  /*! \brief channels of decoded image */
  int channels = 3;
  /*! \brief output image data per second or per frame */
  bool second_mode = false;
  /*! \brief only decode top n frame */
  int top_n = -1;
};


class WebPDecoder{
public:
  WebPDecoder() = default;

  /*!
   * decode a webp file specified by decode parameters
   * \param param reference to decode parameters
   * \param decoded_data output image data
   * \param img_info output image info
   * \return -1   file open error
   *         0    decode success
   *         VP8StatusCode see webp/decode.h in libwebp
   */
  int Decode(DecodeWebPParam &param, std::vector<uint8_t> &decoded_data, ImageInfo &img_info);

  /*!
   * decode a webp file specified by memory data
   * \param img_data input image data
   * \param data_size input image data size
   * \param decoded_data output image data
   * \param img_info output image info
   * \return -1   file open error
   *         0    decode success
   *         VP8StatusCode see webp/decode.h in libwebp
   */
  int Decode(const uint8_t *img_data, const size_t data_size, const bool second_mode, std::vector<uint8_t> &decoded_data, ImageInfo &img_info);

  /*!
   * decode a webp file specified by file name
   * \param img_data input image data
   * \param decoded_data output image data
   * \param img_info output image info
   * \return -1   file open error
   *         0    decode success
   *         VP8StatusCode see webp/decode.h in libwebp
   */
  int Decode(const char *fname, const bool second_mode, std::vector<uint8_t> &decoded_data, ImageInfo &img_info);
};