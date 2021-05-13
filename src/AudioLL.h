
/* Specifies the interface. */
#ifndef __AUDIOLL_H__
#define __AUDIOLL_H__

#include <gflags/gflags.h>
#include <blockingconcurrentqueue.h>
#include <brpc/channel.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include <exception>
#include <ffmpeg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <type_traits>

// Read Audio
#include <fstream>
// FFT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sndfile.h>
#include <iostream>
#include <sstream>
#include <vector>
// fftw
#include "fftw-3.3.8/fftw3.h"
// Duration
#include <bits/stdc++.h>
#include <sys/time.h>
#include <stack>
#include <ctime>
#include <unistd.h>
// Misc
#include <numeric>
// Enable ffmpeg to read mp3
// https://rodic.fr/blog/libavcodec-tutorial-decode-audio-file/
// https://blog.csdn.net/chinazjn/article/details/7954984
#include "fir_api.h"
#include "typedef.h"

/* -------------------------------------- Misc -------------------------------------- */
double get_duration(struct timeval start, struct timeval end);


/* -------------------------------------- Read Audio -------------------------------------- */
// Read in a fixed duration, from a buffer array by referenc
void read_audio_double(std::string filePath_str, int AUDIO_DURATION, double * output_buffer);
// Read in variable duration, output buffe
void GetAudioInformation(std::string filePath_str, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length);

/* -------------------------------------- FFT -------------------------------------- */
void computeWindow(int nfft, double windowFunction[], std::string windowType);
// void computeFullFFT_vector(int numFrames, int nfft, int hop_length, std::vector<double> &buffer, double windowFunction[], double * Spectrogram );
// void computeFullFFT_double(int numFrames, int nfft, int hop_length, double * buffer, double windowFunction[], double * Spectrogram );
void computeFullFFT_double_fast(
    int numFrames, int nfft, int hop_length,
    double * buffer, double windowFunction[], double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p);
// void computeFullFFT_short(int numFrames, int nfft, int hop_length, signed short * buffer, double windowFunction[], double * Spectrogram );
void computeFullFFT_short_fast(
    int numFrames, int nfft, int hop_length,
    signed short * buffer, double windowFunction[], double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p);
void computeFullFFT_vector(
    int numFrames, int nfft, int hop_length,
    std::vector<float>& buffer, std::vector<float>& windowFunction, double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p);

/* -------------------------------------- I/O -------------------------------------- */
/* -- Write to file -- */
void wave2txt(const std::string &fileName, signed short * buffer, int AUDIO_DURATION);
void spec2txt(const std::string &fileName, double * Spectrogram, int numFrames, int nfft);
/* -- Read from file -- */
void txt2spec(const std::string &fileName, double * Spectrogram, int numCols);


#endif // __AUDIOLL_H__
