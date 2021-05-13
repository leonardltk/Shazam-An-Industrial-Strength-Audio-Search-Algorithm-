#pragma once

#include "Shash.h"
#include <mutex>

extern "C" {
#include <libswresample/swresample.h>
}

class HalfResampler {
public:
    HalfResampler(int in_sample_rate, int out_sample_rate);
    ~HalfResampler();

    bool resample(const std::vector<float>& input, std::vector<float>& output);

private:
    struct SwrContext* mSwrCtx = nullptr;
    int mInSR = 16000;
    int mOutSR = 8000;
    bool mReady = false;
};

// Warning, this class is not thread-safe!
class AudioSHasher {
public:
    AudioSHasher();
    ~AudioSHasher();

    static void GenHanningWindow(std::vector<float>& window);
    static void downsample(std::vector<float>& audioIn, std::vector<float>& buffer_8k);

    bool get_shash(
        std::vector<float>& audio_data,
        int sample_rate,
        float threshold_abs,
        std::unordered_map <int, std::set<int>>& out_yhash);

    bool get_shashes(
        std::vector<float>& audio_data,
        int sample_rate,
        float threshold_abs,
        std::vector< std::unordered_map <int, std::set<int>> > &out_yhash);

private:
    /* fft params */
    static const int nfft = 512;
    static const int hop_length = 256;
    static const int FIR_TAP_NUM = 15;
    static const int INPUT_SIZE = 256;
    static const int OUTPUT_SIZE = 128;
    std::vector<float> windowFunction;

    /* complex array to hold fft data */
    fftw_complex* mFFTIn = nullptr;
    fftw_complex* mFFTOut = nullptr;
    fftw_plan mFFTPlan;
    static std::mutex mFFTWLock;

    /* shash params */
    static const int MAXLISTWinSize = 7;
    // static const float threshold_db =  1;       // thabs_db = 1
    // static const float threshold_qry = 8192;    // thabs_qry=8192 : Accounted for short int, instead of float (0.25 -> 8192)

};
