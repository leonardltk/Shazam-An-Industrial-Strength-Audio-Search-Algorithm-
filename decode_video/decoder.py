from __future__ import absolute_import

import os
import sys
import ctypes
import logging
import warnings
import numpy as np

__all__ = ["Decoder"]

if sys.version_info[0] == 3:
    py_str = lambda x: x.decode('utf-8')
else:
    py_str = lambda x: x

def c_str(string):
  """"Convert a python string to C string."""
  if not isinstance(string, str):
    string = string.decode('ascii')
  return ctypes.c_char_p(string.encode('utf-8'))


def c_array(ctype, values):
  """Create ctypes array from a python array."""
  return (ctype * len(values))(*values)


def _find_lib_path():
  this_dir = os.path.dirname(os.path.realpath(__file__))
  search_relative_dirs = [".", "build", "build/lib", "../build/lib", "../build", "../lib"]
  search_dirs = list(map(lambda p: os.path.join(this_dir, p), search_relative_dirs))
  if 'DECODE_LIB_PATH' in os.environ:
    search_dirs.insert(0, os.environ['DECODE_LIB_PATH'])
  for search_dir in search_dirs:
    amalgamation_lib_path = os.path.join(search_dir, 'libdecode_video.so')
    if os.path.exists(amalgamation_lib_path) and os.path.isfile(amalgamation_lib_path):
      lib_path = [amalgamation_lib_path]
      return lib_path
  raise RuntimeError('Cannot find libdecode_video.so')


def _load_lib():
  """Load libary by searching possible path."""
  lib_path = _find_lib_path()
  lib = ctypes.cdll.LoadLibrary(lib_path[0])
  return lib


def _check_all(ret):
  """Check the return value of API."""
  if ret != 0:
    raise RuntimeError("Failed!")

_LIB = _load_lib()
# type definitions
bigo_int = ctypes.c_int
bigo_uint = ctypes.c_uint
bigo_uint_p = ctypes.POINTER(bigo_uint)
bigo_uint8 = ctypes.c_uint8
bigo_uint8_p = ctypes.POINTER(bigo_uint8)
bigo_float = ctypes.c_float
bigo_float_p = ctypes.POINTER(bigo_float)
DecoderHandle = ctypes.c_void_p

class Decoder(object):
  # dev_id : -1, use cpu; 0, use gpu0, 1, use gpu1 ...
  def __init__(self, dev_id=-1, use_nvidia_sdk = 0, second_mode=0, req_sample_rate=22050, only_audio=False):
    self.dev_id = dev_id
    self.req_sample_rate = req_sample_rate
    handle = DecoderHandle()
    _check_all(_LIB.BigoMLCreateDecoder(ctypes.byref(handle), bigo_int(dev_id), bigo_int(second_mode), bigo_int(use_nvidia_sdk), bigo_int(only_audio)))
    if(req_sample_rate > 0):
      _check_all(_LIB.BigoMLSetAudioParam(handle, bigo_int(req_sample_rate)))
    else:
      _check_all(_LIB.BigoMLDisableAudio(handle))
    self.handle = handle

  def __del__(self):
    _check_all(_LIB.BigoMLDelDecoder(self.handle))

  def set_input(self, video):
    if len(video) == 0 or video is None:
      raise ValueError("Input is empty.")
    v = np.ascontiguousarray(video, dtype=np.uint8)
    _check_all(_LIB.BigoMLDecodeVideo(self.handle,
                                      v.ctypes.data_as(bigo_uint8_p),
                                      bigo_uint(v.size)))

  def get_video_zero_copy(self):
    shape = np.empty([4,], dtype=np.uint32)
    _check_all(_LIB.BigoMLGetShape(self.handle,
                                   shape.ctypes.data_as(bigo_uint_p),
                                   bigo_uint(4)))

    ptr = bigo_uint8_p()
    size = bigo_uint()
    _check_all(_LIB.BigoMLGetOutput(self.handle,
                                    ctypes.byref(ptr),
                                    ctypes.byref(size)))
    warnings.filterwarnings('ignore')
    if not ptr:
      return None
    ret = np.ctypeslib.as_array(ptr, shape=tuple(shape) )
    return ret

  def get_audio_zero_copy(self):
    ptr = bigo_float_p()
    size = bigo_uint()
    if self.req_sample_rate < 0:
      return None
    _check_all(_LIB.BigoMLGetAudio(self.handle,
                                   ctypes.byref(ptr),
                                   ctypes.byref(size)))
    if size.value:
      ret = np.ctypeslib.as_array(ptr, shape=(size.value,) )
    else:
      ret = np.array([])
    return ret

  def get_vinfo(self):
    vinfo = {}
    pts_size = ctypes.c_uint32()
    pts_ptr = ctypes.POINTER(ctypes.c_int64)()
    _check_all(_LIB.BigoMLGetVideoPTS(self.handle,
                                      ctypes.byref(pts_ptr),
                                      ctypes.byref(pts_size)))
    if pts_size.value:
      res = np.ctypeslib.as_array(pts_ptr, shape=(pts_size.value,) )
      vinfo["video_pts_usec"] = res.copy().tolist()

    dts_size = ctypes.c_uint32()
    dts_ptr = ctypes.POINTER(ctypes.c_int64)()
    _check_all(_LIB.BigoMLGetVideoDTS(self.handle,
                                      ctypes.byref(dts_ptr),
                                      ctypes.byref(dts_size)))
    if dts_size.value:
      res = np.ctypeslib.as_array(dts_ptr, shape=(dts_size.value,) )
      vinfo["video_dts_usec"] = res.copy().tolist()

    fps = ctypes.c_float()
    _check_all(_LIB.BigoMLGetFPS(self.handle,
                                 ctypes.byref(fps)))
    vinfo["fps"] = float(fps.value)

    first_dts = ctypes.c_int64()
    _check_all(_LIB.BigoMLGetVideoFirstDTS(self.handle,
                                           ctypes.byref(first_dts)))
    vinfo["first_dts"] = int(first_dts.value)

    duration = ctypes.c_float()
    _check_all(_LIB.BigoMLGetDuration(self.handle,
                                      ctypes.byref(duration)))
    vinfo["duration"] = float(duration.value)

    kbps = ctypes.c_float()
    _check_all(_LIB.BigoMLGetKbps(self.handle,
                                 ctypes.byref(kbps)))
    vinfo["kbps"] = float(kbps.value)

    shape = np.empty([4,], dtype=np.uint32)
    _check_all(_LIB.BigoMLGetShape(self.handle,
                                   shape.ctypes.data_as(bigo_uint_p),
                                   bigo_uint(4)))
    vinfo["height"] = int(shape[1])
    vinfo["width"] = int(shape[2])
    return vinfo

  def get_output(self):
    video = self.get_video_zero_copy()
    audio = self.get_audio_zero_copy()
    vinfo = self.get_vinfo()
    return (video, audio, vinfo)

  def decode(self, video):
    self.set_input(video)
    return self.get_output()

  def clear_buffer(self):
    _check_all(_LIB.BigoMLClearBuffer(self.handle))
    return True
    

if __name__ == '__main__':
  import sys
  import cv2
  if len(sys.argv) < 2:
    print("Usage: python {0} <input_video>.".format(sys.argv[0]))
    exit(-1)
  #decoder = Decoder(dev_id=0, use_nvidia_sdk=0)

  decoder = Decoder(second_mode=1, req_sample_rate=-1)
  decoder = Decoder(second_mode=1)
  #decoder = Decoder(dev_id=0, use_nvidia_sdk=1, second_mode=1)

  for i in range(1, len(sys.argv)):
    print("++++{0}".format(sys.argv[i]))
    data = np.fromfile(sys.argv[i], dtype='uint8')
    try:
      imgs, audio, vinfo = decoder.decode(data)
      vinfo = decoder.get_vinfo()
      #np.set_printoptions(threshold=np.nan)
      if imgs is None:
        img_shape = None
      else:
        img_shape = imgs.shape
      if audio is None:
        audio_shape = None
      else:
        audio_shape = audio.shape
      print("Images shape: {0}, audio shape: {1}".format(img_shape, audio_shape))
      print(vinfo)
      for i in range(img_shape[0]):
        cv2.imwrite("{}.jpg".format(i), imgs[i])
    except Exception as err:
      print(err)

