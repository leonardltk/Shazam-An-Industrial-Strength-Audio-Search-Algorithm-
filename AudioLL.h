
/* Specifies the interface. */
#ifndef __AUDIOLL_H__
#define __AUDIOLL_H__

/* -------------------------------------- imports -------------------------------------- */
#include "AudioLL.h"
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
#include "utils/fftw-3.3.8/api/fftw3.h"
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

using namespace std;

/* -------------------------------------- Misc -------------------------------------- */
double get_duration(struct timeval start, struct timeval end);


/* -------------------------------------- Read Audio -------------------------------------- */
// Read in a fixed duration, from a buffer array by referenc
void read_audio_double(string filePath_str, int AUDIO_DURATION, double * output_buffer);
// Read in variable duration, output buffe
void GetAudioInformation(string filePath_str, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length);
// Read in a mp3 file using ffmpe
int read_audio_mp3(string filePath_str, const int sample_rate, double** output_buffer, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length);
// Reduce memory leak : av_free(buffer) & av_free_packet(&packet)
int read_audio_mp3_free(string filePath_str, const int sample_rate, double** output_buffer, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length);
int getlongbuffer(vector<short> &buffer, int &AUDIO_DURATION, string wavpath);


/* -------------------------------------- FFT -------------------------------------- */
void computeWindow(int nfft, double windowFunction[], string windowType);
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
    vector<short>& buffer, double windowFunction[], double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p);

/* -------------------------------------- I/O -------------------------------------- */
/* -- Write to file -- */
void wave2txt(const string &fileName, signed short * buffer, int AUDIO_DURATION);
void spec2txt(const string &fileName, double * Spectrogram, int numFrames, int nfft);
/* -- Read from file -- */
void txt2spec(const string &fileName, double * Spectrogram, int numCols);


#endif // __AUDIOLL_H__
