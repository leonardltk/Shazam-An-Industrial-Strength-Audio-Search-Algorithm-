/*!
 * Copyright (c) 2018 by Contributors
 * \file util.hpp
 * \author zhuangrongbin
 * \brief
*/
#pragma once

#include <vector>

struct VideoInfo {
  /*! \brief image channel */
  int channels;
  /*! \brief image height */
  int height;
  /*! \brief image width */
  int width;
  /*! \brief output a image per step */
  float step;
  /*! \brief video duration(second) */
  float duration;
  /*! \brief frame per second */
  float fps;
  /*! \brief useful in second_mode */
  std::vector<int> frame_ind;   

  int video_kbps;
  std::vector<int64_t> video_pts_usec;
  std::vector<int64_t> video_dts_usec;
  int64_t first_dts;
  int codec;
};
