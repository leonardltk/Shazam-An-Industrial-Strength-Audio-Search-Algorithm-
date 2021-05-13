#include "shash_class.h"

HalfResampler::HalfResampler(int in_sr, int out_sr) : mInSR(in_sr), mOutSR(out_sr) {
    mSwrCtx = swr_alloc_set_opts(NULL,
                                 AV_CH_LAYOUT_MONO, // out_ch_layout
                                 AV_SAMPLE_FMT_FLT, // out_sample_fmt
                                 mOutSR,            // out_sample_rate
                                 AV_CH_LAYOUT_MONO, // in_ch_layout
                                 AV_SAMPLE_FMT_FLT, // in_sample_fmt
                                 mInSR,             // in_sample_rate
                                 0,                 // log_offset
                                 NULL);             // log_ctx
    if (!mSwrCtx) {
        printf(" Failed to alloc SwsContext\n");
    }
    else {
        int ret_code = swr_init(mSwrCtx);
        if (ret_code < 0) {
            printf(" Failed to init SwsContext\n");
        }
    }
    mReady = true;
}

HalfResampler::~HalfResampler() {
    if (mSwrCtx) {
        swr_free(&mSwrCtx);
    }
}

bool HalfResampler::resample(const std::vector<float>& input, std::vector<float>& output) {
    if (!mReady) {
        return false;
    }
    int out_samples = (int)swr_get_delay(mSwrCtx, mOutSR) + swr_get_out_samples(mSwrCtx, int(input.size()));
    output.resize(size_t(out_samples));
    memset(output.data(), 0, sizeof(float) * out_samples);
    uint8_t* outbuf = (uint8_t*)output.data();
    const uint8_t* inbuf = (uint8_t*)input.data();
    auto output_samples =
            swr_convert(mSwrCtx, (uint8_t**)&outbuf, out_samples, (const uint8_t**)&inbuf, int(input.size()));
    if (output_samples < 0) {
        printf("swr_convert failed, return %d\n", output_samples);
        return false;
    }
    if (output_samples < int(output.size())) {
        output.resize(size_t(output_samples));
    }
    return true;
}

std::mutex AudioSHasher::mFFTWLock;

AudioSHasher::AudioSHasher() {
    printf("\t%s() : \n", __func__);
    printf("\t%s() : init lock(mFFTWLock)\n", __func__);
    windowFunction.resize(nfft);
    GenHanningWindow(windowFunction);
    {
        std::lock_guard<std::mutex> lock(mFFTWLock);
        /* Init (complex array to hold fft data) */
        mFFTIn  = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
        mFFTOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
        mFFTPlan = fftw_plan_dft_1d (nfft, mFFTIn, mFFTOut, FFTW_FORWARD, FFTW_ESTIMATE);
    }
    printf("\t%s() : --- fft params ---\n", __func__);
    printf("\t%s() :     nfft           = %i\n", __func__, nfft);
    printf("\t%s() :     hop_length     = %i\n", __func__, hop_length);
    // printf("\t%s() : --- Downsampling Params ---\n", __func__);
    // printf("\t%s() :     FIR_TAP_NUM    = %i\n", __func__, FIR_TAP_NUM);
    // printf("\t%s() :     INPUT_SIZE     = %i\n", __func__, INPUT_SIZE);
    // printf("\t%s() :     OUTPUT_SIZE    = %i\n", __func__, OUTPUT_SIZE);
    printf("\t%s() : --- shash params ---\n", __func__);
    printf("\t%s() :     MAXLISTWinSize = %i\n", __func__, MAXLISTWinSize);
}

void AudioSHasher::GenHanningWindow(std::vector<float>& window) {
    printf("\t%s() : Using Hanning window\n", __func__);
    if (window.size() != nfft) {
        window.resize(nfft);
    }
    double atom=2.*M_PI/(nfft-1);
    for (int i = 0; i < nfft; i++){
        window[i] = 0.5 * (1 - cos(atom*i));
    }
}

/* this is the original downsampler used for integer audio inputs, not used here */
void AudioSHasher::downsample(std::vector<float>& audioIn, std::vector<float>& buffer_8k) {
    /* Try to attempt Shazan_querying method to do downsampling but it doesnt work */
    printf("\t%s() : \n", __func__);
    /* ============== For downsampling audio ============== */
        int FixedDuration = 20*8000;
        // for FIR
        S16 aswCoeff[FIR_TAP_NUM] = {-1477, 1206, 2225, -233, -3041, 754, 10486, 15894, 10486, 754, -3041, -233, 2225, 1206, -1477 }; // fpass=3400, fstop=4300 @ fs=16000
        S16 aswStateMic1[FIR_TAP_NUM + INPUT_SIZE];
        // for evaluation
        short aswMic1[INPUT_SIZE];
        short aswMic1_Out[INPUT_SIZE];
        // Init Downsampler
        FIR_Q15_INSTANCE tFIRMic1;
        FIR_Q15_INIT(&tFIRMic1, FIR_TAP_NUM, aswCoeff, aswStateMic1, INPUT_SIZE);
    /* ================================================= */
        unsigned i1;
        int AUDIO_DURATION=0;
        while (1) {
            // read chunk
            for (i1 = 0; i1 < INPUT_SIZE; i1++){
                aswMic1[i1] = (S16) 32768.*audioIn[AUDIO_DURATION+i1]; // 2^15 = 32768
                // printf(" %i", aswMic1[i1]);
            }
            // filtering
            FIR_Q15(&tFIRMic1, (S16 *)aswMic1, (S16 *)aswMic1_Out, INPUT_SIZE);
            // decimation
            for (i1 = 0; i1 < OUTPUT_SIZE; i1++){
                buffer_8k[AUDIO_DURATION+i1] = (float) aswMic1_Out[2 * i1];
            }
            AUDIO_DURATION += OUTPUT_SIZE;
            // exit if more than 20s
            if (AUDIO_DURATION>(FixedDuration-OUTPUT_SIZE)){
                break;
            }
        }
}

AudioSHasher::~AudioSHasher() {
    printf("\t%s() : \n", __func__);
    std::lock_guard<std::mutex> lock(mFFTWLock);
    fftw_destroy_plan(mFFTPlan);
    if (mFFTIn){
        fftw_free(mFFTIn);
    }
    if (mFFTOut){
        fftw_free(mFFTOut);
    }

    mFFTIn = nullptr;
    mFFTOut = nullptr;
    mFFTPlan = nullptr;
}

bool AudioSHasher::get_shash(
    std::vector<float>& audio_data,
    int sample_rate, 
    float threshold_abs,
    std::unordered_map <int, std::set<int>>& out_shash) {

    /* Compute a single shash from the std::vector of audio
        Inputs
        ------

        audio_data : std::vector<float>&
            Pass the audio data std::vector in by reference.

        sample_rate : int
            sampling rate of the input audio
            By default, it is 16k, and we want to downsample to 8k

        threshold_abs : float
            threshold for peak selection

        Returns
        -------

        out_shash : std::unordered_map <int, std::set<int>>&
            Pass the shash in by reference.
    */

    /* FFT Params */
        int numFrames;
        numFrames = (audio_data.size()-nfft)/hop_length + 1;

    /* Resampling (I) */
        std::vector<float>* in_audio = &audio_data;
        std::vector<float> resample_input;
        if (sample_rate == 16000) {
            HalfResampler hr(16000, 8000);
            if (!hr.resample(audio_data, resample_input)) {
                LOG(ERROR) << __FUNCTION__ << ": Resample failed!";
                return false;
            }
            in_audio = &resample_input;

        }
        size_t inlen = in_audio->size();
        numFrames = (inlen-nfft)/hop_length + 1;

    /* Compute Window for FFT */
        GenHanningWindow(windowFunction);

    /* Compute FFT */
        double *Spectrogram = (double *)malloc(numFrames * nfft/2 * sizeof(double));
        computeFullFFT_vector(numFrames, nfft, hop_length, *in_audio, windowFunction, (double*)Spectrogram,
            (fftw_complex*)mFFTIn, (fftw_complex*)mFFTOut, (fftw_plan)mFFTPlan);

    /* Compute Shashing */
        out_shash = spec2shash_v4(
            (double*)Spectrogram, numFrames, nfft,
            MAXLISTWinSize, threshold_abs);

    /* Free variables */
        free(Spectrogram);

    return true;
}

bool AudioSHasher::get_shashes(
    std::vector<float>& audio_data,
    int sample_rate,
    float threshold_abs,
    std::vector< std::unordered_map <int, std::set<int>> > &out_shashes) {
    /* Compute a single shash from the std::vector of audio
        Inputs
        ------

        audio_data : std::vector<float>&
            Pass the audio data std::vector in by reference.

        threshold_abs : float
            threshold for peak selection

        Returns
        -------

        out_shashes : std::vector< std::unordered_map <int, std::set<int>> > *
            Pass the out_shashes in by reference.
    */

    LOG(INFO) << "\t" << __func__ << "(): ";

    /* FFT Params */
        int numFrames;
        numFrames = (audio_data.size()-nfft)/hop_length + 1;
        LOG(INFO) << "\t" << __func__ << "(): numFrames(before) = " << numFrames;
        LOG(INFO) << "\t" << __func__ << "(): threshold_abs     = " << threshold_abs;
        LOG(INFO) << "\t" << __func__ << "(): sample_rate       = " << sample_rate;
        LOG(INFO) << "\t" << __func__ << "(): audio_data(min,max) = "
            << *min_element(audio_data.begin(), audio_data.end())
            << ", "
            << *max_element(audio_data.begin(), audio_data.end());

    /* Resampling (I) */
        LOG(INFO) << "\t" << __func__ << "(): Resampling...";
        std::vector<float>* in_audio = &audio_data;
        std::vector<float> resample_input;
        if (sample_rate == 16000) {
            HalfResampler hr(16000, 8000);
            if (!hr.resample(audio_data, resample_input)) {
                LOG(INFO) << "\t" << __func__ << "(): Resample failed!";
                return false;
            }
            in_audio = &resample_input;
            LOG(INFO) << "\t" << __func__ << "(): Resampling done!";

        }
        size_t inlen = in_audio->size();
        numFrames = (inlen-nfft)/hop_length + 1;
        LOG(INFO) << "\t" << __func__ << "(): numFrames(after) = " << numFrames;
        LOG(INFO) << "\t" << __func__ << "(): audio_data(min,max) = "
            << *min_element(audio_data.begin(), audio_data.end())
            << ", "
            << *max_element(audio_data.begin(), audio_data.end());

    /* Compute Window for FFT */
        GenHanningWindow(windowFunction);

    /* Split them into 1min chunks */
    int TotalSamples = in_audio->size();
    int SegmentLength = 60*8000;
    int SegmentHop = 30*8000;
    int NumberOfHashes = 1 + std::max((TotalSamples-SegmentLength)/SegmentHop, 0);
    int startIdx, endIdx, HashIdx;
    std::vector<float>::const_iterator first, last ;
    for (HashIdx=1; HashIdx<=NumberOfHashes; HashIdx++){
        /* Crop the audio */
            startIdx = (HashIdx-1)*SegmentHop;
            if (HashIdx<NumberOfHashes) {
                endIdx = startIdx+SegmentLength;
            } else {
                endIdx = TotalSamples;
            }
            std::vector<float> Current_Audio_Segment(in_audio->begin() + startIdx, in_audio->begin() + endIdx );
            printf("\t%s() : Current_Audio_Segment length = %lu \n", __func__, Current_Audio_Segment.size());

        /* Compute FFT */
            double *Spectrogram = (double *)malloc(numFrames * nfft/2 * sizeof(double));
            computeFullFFT_vector(numFrames, nfft, hop_length, Current_Audio_Segment, windowFunction, (double*)Spectrogram,
                (fftw_complex*)mFFTIn, (fftw_complex*)mFFTOut, (fftw_plan)mFFTPlan);

        /* Compute Shashing */
            std::unordered_map <int, std::set<int>> out_shash = spec2shash_v4(
                (double*)Spectrogram, numFrames, nfft,
                MAXLISTWinSize, threshold_abs);
            out_shashes.push_back(out_shash);
            LOG(INFO) << "\t" << __func__ << "(): Compute Shashing -> out_shash.size() = " << out_shash.size();

        /* Free variables */
            free(Spectrogram);
    }
    printf("\t%s() : out_shashes.size() = %lu \n", __func__, out_shashes.size());

    LOG(INFO) << "\t" << __func__ << "(): Done.";

    return true;
}
