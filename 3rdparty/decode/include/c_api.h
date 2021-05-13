#pragma once
extern "C" {
#include <stdio.h>
#include <stdint.h>
  typedef void* DecoderHandle;
  
  int BigoMLCreateDecoder(DecoderHandle *handle, int dev_id, int second_mode, int use_naive_cuvid, int only_audio);
  
  int BigoMLDelDecoder(DecoderHandle handle);
   
  int BigoMLDisableAudio(DecoderHandle handle);

  int BigoMLSetAudioParam(DecoderHandle handle, int req_sample_rate);
  
  int BigoMLDecodeVideo(DecoderHandle handle, const uint8_t* input_data, uint32_t input_size);
  
  int BigoMLGetShape(DecoderHandle handle, uint32_t* shape_data, uint32_t shape_dim);
  
  int BigoMLGetOutput(DecoderHandle handle, uint8_t** data, uint32_t *size);
  
  int BigoMLGetAudio(DecoderHandle handle, float **data, uint32_t *size);

  int BigoMLClearBuffer(DecoderHandle handle);

  /*----------------VideoInfo filed ----------------*/
  int BigoMLGetFPS(DecoderHandle handle, float *fps);

  int BigoMLGetKbps(DecoderHandle handle, float *kbps);

  int BigoMLGetDuration(DecoderHandle handle, float *duration);
   
  int BigoMLGetVideoFirstDTS(DecoderHandle handle, int64_t *first_dts);

  int BigoMLGetVideoPTS(DecoderHandle handle, int64_t **pts, uint32_t *size);

  int BigoMLGetVideoDTS(DecoderHandle handle, int64_t **dts, uint32_t *size);

}
