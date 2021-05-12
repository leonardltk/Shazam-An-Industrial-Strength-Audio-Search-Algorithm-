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
#include "fir_api.h"
#include "typedef.h"
// Enable ffmpeg to read mp3
// https://rodic.fr/blog/libavcodec-tutorial-decode-audio-file/
// https://blog.csdn.net/chinazjn/article/details/7954984
extern "C"
{
    #include <libavutil/opt.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

using namespace std;

#define ARRAY_LEN(x)    ((int) (sizeof (x) / sizeof (x [0])))
#define MAX(x,y)        ((x) > (y) ? (x) : (y))
#define MIN(x,y)        ((x) < (y) ? (x) : (y))

/* -------------------------------------- Misc -------------------------------------- */
double get_duration(struct timeval start, struct timeval end) {
    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
    return time_taken;
}


/* -------------------------------------- Read Audio -------------------------------------- */
// Read in a fixed duration, from a buffer array by reference
void read_audio_double(string filePath_str, int AUDIO_DURATION, double * output_buffer){
    const char* filePath = filePath_str.c_str();

    SNDFILE *infile ;
    SF_INFO sfinfo ;

    double *buffer = (double *)malloc(AUDIO_DURATION * sizeof(double));

    memset (&sfinfo, 0, sizeof (sfinfo)) ;
    if ((infile = sf_open (filePath, SFM_READ, &sfinfo)) == NULL)     {
        printf ("Error : Not able to open input file '%s'\n", filePath);
        sf_close (infile);
        exit (1) ;
    }

    sf_read_double (infile, buffer, AUDIO_DURATION);
    for (int j=0; j<AUDIO_DURATION; ++j){
        output_buffer[j] = buffer[j];
    }
    sf_close (infile) ;

    free(buffer);
}
// Read in variable duration, output buffer
void GetAudioInformation(string filePath_str, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length){
    const char* filePath = filePath_str.c_str();
    SNDFILE *infile ;
    SF_INFO sfinfo ;

    /* Open the WAV file. */
    sfinfo.format = 0;
    infile = sf_open(filePath,SFM_READ,&sfinfo);
    if (infile == NULL)
    {
        printf("Failed to open the file.\n");
        throw;
    }

    /* Print some of the sfinfo, and figure out how much data to read. */
    int frames = sfinfo.frames; // printf("frames=%d\n",frames);
    // int samplerate = sfinfo.samplerate; // printf("samplerate=%d\n",samplerate);
    // int channels = sfinfo.channels; // printf("channels=%d\n",channels);
    // int num_items = frames*channels; // printf("num_items=%d\n",num_items);

    /* Get AUDIO_DURATION, numFrames.
    Pads such that AUDIO_DURATION can be round off to the next frame */
    int unpadded = (frames-nfft) % hop_length;
    AUDIO_DURATION = frames;
    if (unpadded>0){
        AUDIO_DURATION -= unpadded; // To truncate the end
        // AUDIO_DURATION += hop_length - unpadded; // To right pad the end
    }
    numFrames = (AUDIO_DURATION-nfft)/hop_length + 1;
}
// Read in a mp3 file using ffmpeg
int read_audio_mp3(string filePath_str, const int sample_rate, double** output_buffer, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length){
    const char* path = filePath_str.c_str();

    /* This function reads the file header and stores information about the file format in the
        AVFormatContext structure we have given it.
        The last three arguments are used to specify the file format, buffer AUDIO_DURATION, and format options,
        but by setting this to NULL or 0, libavformat will auto-detect these.
    */
    AVFormatContext* format = avformat_alloc_context();
    if (avformat_open_input(&format, path, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file '%s'\n", path);
        return -1;
    }

    /* Check out the stream information in the file. */
    if (avformat_find_stream_info(format, NULL) < 0) {
        fprintf(stderr, "Could not retrieve stream info from file '%s'\n", path);
        return -1;
    }

    /* handy debugging function to show us what's inside. */
    // av_dump_format(format, 0, path, 0);

    /* format->streams is just an array of pointers,
        of AUDIO_DURATION format->nb_streams,
        walk through it until we find an audio stream.
    */
    int stream_index =- 1;
    for (unsigned i=0; i<format->nb_streams; i++) {
        if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
      }
    if (stream_index == -1) {
        fprintf(stderr, "Could not retrieve audio stream from file '%s'\n", path);
        return -1;
    }
    AVStream* stream = format->streams[stream_index];


    /* find & open codec
        The stream's information about the codec is in what we call the "codec context."
        This contains all the information about the codec that the stream is using,
        and now we have a pointer to it.
        But we still have to find the actual codec and open it.
    */
    AVCodecContext* codec = stream->codec;
    if (avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), NULL) < 0) {
        fprintf(stderr, "Failed to open decoder for stream #%u in file '%s'\n", stream_index, path);
        return -1;
    }

    // prepare resampler
    struct SwrContext* swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_count",  codec->channels, 0);
    av_opt_set_int(swr, "out_channel_count", 1, 0);
    av_opt_set_int(swr, "in_channel_layout",  codec->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
    av_opt_set_int(swr, "in_sample_rate", codec->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  codec->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_DBL,  0);
    swr_init(swr);
    if (!swr_is_initialized(swr)) {
        fprintf(stderr, "Resampler has not been properly initialized\n");
        return -1;
    }

    /* prepare to read output_buffer
        Now we need a place to actually store the frame.
        Allocate an audio frame.
    */
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error allocating the frame\n");
        return -1;
    }

    // iterate through frames
    *output_buffer = NULL;
    AUDIO_DURATION = 0;
    while (av_read_frame(format, &packet) >= 0) {
        // decode one frame
        int gotFrame;
        if (avcodec_decode_audio4(codec, frame, &gotFrame, &packet) < 0) {break;}
        if (!gotFrame) {continue;}

        // resample frames
        double* buffer;
        av_samples_alloc((uint8_t**) &buffer, NULL, 1, frame->nb_samples, AV_SAMPLE_FMT_DBL, 0);
        int frame_count = swr_convert(swr, (uint8_t**) &buffer, frame->nb_samples, (const uint8_t**) frame->data, frame->nb_samples);

        // append resampled frames to output_buffer
        *output_buffer = (double*) realloc(*output_buffer,
        (AUDIO_DURATION + frame->nb_samples) * sizeof(double));
        memcpy(*output_buffer + AUDIO_DURATION, buffer, frame_count * sizeof(double));
        AUDIO_DURATION += frame_count;
    }

    // clean up
    av_frame_free(&frame);
    swr_free(&swr);
    avcodec_close(codec);
    avformat_free_context(format);

    numFrames = (AUDIO_DURATION-nfft)/hop_length + 1;
    return 0;
}
// Reduce memory leak : av_free(buffer) & av_free_packet(&packet);
int read_audio_mp3_free(string filePath_str, const int sample_rate, double** output_buffer, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length){
    const char* path = filePath_str.c_str();

    /* This function reads the file header and stores information about the file format in the
    AVFormatContext structure we have given it.
    The last three arguments are used to specify the file format, buffer AUDIO_DURATION, and format options,
    but by setting this to NULL or 0, libavformat will auto-detect these. */
    AVFormatContext* format = avformat_alloc_context();
    if (avformat_open_input(&format, path, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open file '%s'\n", path);
        avformat_free_context(format);
        return -1;
    }

    /* Check out the stream information in the file. */
    if (avformat_find_stream_info(format, NULL) < 0) {
        fprintf(stderr, "Could not retrieve stream info from file '%s'\n", path);
        avformat_free_context(format);
        return -1;
    }

    /* handy debugging function to show us what's inside. */
    // av_dump_format(format, 0, path, 0);

    /* format->streams is just an array of pointers,
    of AUDIO_DURATION format->nb_streams,
    walk through it until we find an audio stream. */
    int stream_index =- 1;
    for (unsigned i=0; i<format->nb_streams; i++) {
        if (format->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }
    if (stream_index == -1) {
        fprintf(stderr, "Could not retrieve audio stream from file '%s'\n", path);
        avformat_free_context(format);
        return -1;
    }
    AVStream* stream = format->streams[stream_index];


    // find & open codec
    /* The stream's information about the codec is in what we call the "codec context."
    This contains all the information about the codec that the stream is using,
    and now we have a pointer to it.
    But we still have to find the actual codec and open it. */
    AVCodecContext* codec = stream->codec;
    if (avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), NULL) < 0) {
        fprintf(stderr, "Failed to open decoder for stream #%u in file '%s'\n", stream_index, path);
        avformat_free_context(format);
        avcodec_close(codec);
        return -1;
    }

    // prepare resampler
    struct SwrContext* swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_count",  codec->channels, 0);
    av_opt_set_int(swr, "out_channel_count", 1, 0);
    av_opt_set_int(swr, "in_channel_layout",  codec->channel_layout, 0);
    av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
    av_opt_set_int(swr, "in_sample_rate", codec->sample_rate, 0);
    av_opt_set_int(swr, "out_sample_rate", sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt",  codec->sample_fmt, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_DBL,  0);
    swr_init(swr);
    if (!swr_is_initialized(swr)) {
        fprintf(stderr, "Resampler has not been properly initialized\n");
        avformat_free_context(format);
        avcodec_close(codec);
        swr_free(&swr);
        return -1;
    }

    /* prepare to read output_buffer
    Now we need a place to actually store the frame.
    Allocate an audio frame. */
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error allocating the frame\n");
        avformat_free_context(format);
        avcodec_close(codec);
        swr_free(&swr);
        av_free(&frame);
        return -1;
    }

    // iterate through frames
    *output_buffer = NULL;
    AUDIO_DURATION = 0;
    while (av_read_frame(format, &packet) >= 0) {
    // decode one frame
        int gotFrame;
        if (avcodec_decode_audio4(codec, frame, &gotFrame, &packet) < 0) {
    // free packet
            av_free_packet(&packet);
            break;
        }
        if (!gotFrame) {
    // free packet
            av_free_packet(&packet);
            continue;
        }
    // resample frames
        double* buffer;
        av_samples_alloc((uint8_t**) &buffer, NULL, 1, frame->nb_samples, AV_SAMPLE_FMT_DBL, 0);
        int frame_count = swr_convert(swr, (uint8_t**) &buffer, frame->nb_samples, (const uint8_t**) frame->data, frame->nb_samples);
    // append resampled frames to output_buffer
        *output_buffer = (double*) realloc(*output_buffer,
            (AUDIO_DURATION + frame->nb_samples) * sizeof(double));
        memcpy(*output_buffer + AUDIO_DURATION, buffer, frame_count * sizeof(double));
        AUDIO_DURATION += frame_count;
    // free buffer & packet
        av_free_packet(&packet);
        av_free(buffer);
    }

    // clean up
    avformat_free_context(format);
    avcodec_close(codec);
    swr_free(&swr);
    av_frame_free(&frame);

    numFrames = (AUDIO_DURATION-nfft)/hop_length + 1;
    return 0;
}

int getlongbuffer(vector<short> &buffer, int &AUDIO_DURATION, string wavpath) {
    unsigned FIR_TAP_NUM = 15;
    unsigned INPUT_SIZE = 256;
    unsigned OUTPUT_SIZE = 128;
    unsigned i1;
    S16 aswCoeff[FIR_TAP_NUM] = {-1477, 1206, 2225, -233, -3041, 754, 10486, 15894, 10486, 754, -3041, -233, 2225, 1206, -1477 }; // fpass=3400, fstop=4300 @ fs=16000
    S16 aswStateMic1[FIR_TAP_NUM + INPUT_SIZE];
    FILE* pfMic;
    short aswMic1[INPUT_SIZE];
    short aswMic1_Out[INPUT_SIZE];
    FIR_Q15_INSTANCE tFIRMic1;
    FIR_Q15_INIT(&tFIRMic1, FIR_TAP_NUM, aswCoeff, aswStateMic1, INPUT_SIZE);
    // 
    if ((pfMic = fopen(wavpath.c_str(), "rb")) == NULL) {
        printf ("\tError : Not able to open input file '%s'\n", wavpath.c_str());
        fclose(pfMic);
        return 1;
    }
    // 
    buffer.reserve( 3*60*8000 );
    AUDIO_DURATION=0;
    while (1) {

        // read chunk
        if (fread(aswMic1, sizeof(S16), (INPUT_SIZE), pfMic) != (INPUT_SIZE)) break;

        // filtering
        FIR_Q15(&tFIRMic1, (S16 *)aswMic1, (S16 *)aswMic1_Out, INPUT_SIZE);
        // decimation
        for (i1 = 0; i1 < OUTPUT_SIZE; i1++)
            // buffer_8k[AUDIO_DURATION+i1] = aswMic1_Out[2 * i1];
            buffer.push_back( aswMic1_Out[2*i1] );
        AUDIO_DURATION += OUTPUT_SIZE;

        // exit if more than 20s
        // if (AUDIO_DURATION>(FixedDuration-OUTPUT_SIZE)){
        //     break;
        // }
    }
    fclose(pfMic);
    return 0;
}

/* -------------------------------------- FFT -------------------------------------- */
void computeWindow(int nfft, double windowFunction[], string windowType){
    if (windowType == "Hanning"){
        double atom=2.*M_PI/(nfft-1);
        for (int i = 0; i < nfft; i++){
            windowFunction[i] = 0.5 * (1 - cos (atom*i));
        }
    } else if (windowType == "Hamming"){
        double atom=2.*acos(-1.)/(nfft-1); // PI = acos(-1.);
        for (int i = 0; i < nfft; i++){
            windowFunction[i] = 0.54 - 0.46*cos(atom*i);
        }
    } else if (windowType == "Ones"){
        fill_n(windowFunction, nfft, 1);
    } else {
        printf("%s() : No such window exists\n", __func__);
        throw;
    }
    printf("%s() : Using %s window\n", __func__, windowType.c_str());
}
void computeFullFFT_double(int numFrames, int nfft, int hop_length, double * buffer, double windowFunction[], double * Spectrogram ){
    // complex array to hold fft data
    fftw_complex* fftIn  = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_complex* fftOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_plan p;

    int COLS = nfft/2;
    // FFT plan initialisation
    p = fftw_plan_dft_1d (nfft, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);

    // performFFT & Update Spectrogram
    for (int frameidx=0; frameidx<numFrames; ++frameidx){
        // Spectrogram[frameidx] = fft_single_frame(nfft, audioFrames[frameidx], windowFunction);

        // Windowing
        for (int i = 0; i < nfft; i++){
            fftIn[i][0] = buffer[frameidx*hop_length + i] * windowFunction[i];
            fftIn[i][1] = 0.0;
        }

        // perform the FFT
        fftw_execute(p);

        // Get Magnitude Spectrum
        for (int i = 0; i < nfft / 2; i++){
            *(Spectrogram+frameidx * COLS+i) = (fftOut[i][0] * fftOut[i][0]) + (fftOut[i][1] * fftOut[i][1]);
        }
    }
    // destroy fft plan
    fftw_destroy_plan (p);
    fftw_free (fftIn);
    fftw_free (fftOut);
}
void computeFullFFT_double_fast(
    int numFrames, int nfft, int hop_length,
    double * buffer, double windowFunction[], double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p){
    // complex array to hold fft data

    int COLS = nfft/2;

    // performFFT & Update Spectrogram
    for (int frameidx=0; frameidx<numFrames; ++frameidx){

        // Windowing
        for (int i = 0; i < nfft; i++){
            fftIn[i][0] = buffer[frameidx*hop_length + i] * windowFunction[i];
            fftIn[i][1] = 0.0;
        }

        // perform the FFT
        fftw_execute(p);

        // Get Magnitude Spectrum
        for (int i = 0; i < nfft / 2; i++){
            *(Spectrogram+frameidx * COLS+i) = (fftOut[i][0] * fftOut[i][0]) + (fftOut[i][1] * fftOut[i][1]);
        }
    }
}
void computeFullFFT_short_fast(
    int numFrames, int nfft, int hop_length,
    signed short * buffer, double windowFunction[], double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p){
    // complex array to hold fft data

    int COLS = nfft/2;

    // performFFT & Update Spectrogram
    for (int frameidx=0; frameidx<numFrames; ++frameidx){

        // Windowing
        for (int i = 0; i < nfft; i++){
            fftIn[i][0] = buffer[frameidx*hop_length + i] * windowFunction[i];
            fftIn[i][1] = 0.0;
        }

        // perform the FFT
        fftw_execute(p);

        // Get Magnitude Spectrum
        for (int i = 0; i < nfft / 2; i++){
            *(Spectrogram+frameidx * COLS+i) = (fftOut[i][0] * fftOut[i][0]) + (fftOut[i][1] * fftOut[i][1]);
        }
    }
}
void computeFullFFT_vector(
    int numFrames, int nfft, int hop_length,
    vector<short>& buffer, double windowFunction[], double * Spectrogram,
    fftw_complex* fftIn, fftw_complex* fftOut, fftw_plan p){
    // complex array to hold fft data

    int COLS = nfft/2;

    // performFFT & Update Spectrogram
    for (int frameidx=0; frameidx<numFrames; ++frameidx){

        // Windowing
        for (int i = 0; i < nfft; i++){
            fftIn[i][0] = buffer[frameidx*hop_length + i] * windowFunction[i];
            fftIn[i][1] = 0.0;
        }

        // perform the FFT
        fftw_execute(p);

        // Get Magnitude Spectrum
        for (int i = 0; i < nfft / 2; i++){
            *(Spectrogram+frameidx * COLS+i) = (fftOut[i][0] * fftOut[i][0]) + (fftOut[i][1] * fftOut[i][1]);
        }
    }
}


/* -------------------------------------- I/O -------------------------------------- */
/* -- Write to file -- */
void wave2txt(const string &fileName, signed short * buffer, int AUDIO_DURATION) {
    printf("%s() : fileName=%s\n", __func__, fileName.c_str());
    printf("\t%s() : AUDIO_DURATION=%i\n", __func__, AUDIO_DURATION);
    ofstream myFile(fileName);
    if (!myFile.is_open()) {
        printf("\t%s() : Failed to open file\n", __func__);
        return;
    }
    printf("\t%s() : Writing to : %s\n", __func__, fileName.c_str());

    for (int idx=0; idx<AUDIO_DURATION; ++idx){
        myFile << *(buffer+idx) << " ";
    }
    myFile << endl;
    myFile.flush();
    printf("\t%s() : Written to : %s\n", __func__, fileName.c_str());
}
void spec2txt(const string &fileName, double * Spectrogram, int numFrames, int nfft) {
    printf("%s() : fileName=%s\n", __func__, fileName.c_str());
    printf("%s() : numFrames=%i\n", __func__, numFrames);
    printf("%s() : nfft=%i\n", __func__, nfft);
    ofstream myFile(fileName);
    if (!myFile.is_open()) {
        printf("%s() : Failed to open file\n", __func__);
        return;
    }
    printf("%s() : \nWriting to : %s\n", __func__, fileName.c_str());

    int COLS = nfft/2;
    for (int frameidx=0; frameidx<numFrames; ++frameidx){
        for (int i = 0; i < nfft / 2; i++){
            myFile << *(Spectrogram+frameidx * COLS+i) << " ";
        }
        myFile << endl;
    }
    myFile.flush();
    printf("%s() : Written to : %s\n", __func__, fileName.c_str());
}
/* -- Read from file -- */
void txt2spec(const string &fileName, double * Spectrogram, int numCols) {
    ifstream myStream(fileName);
    if (!myStream.is_open()) {
        printf("%s() : Failed to open %s \n", __func__, fileName.c_str());
        throw;
    }

    int rdx=0;
    int cdx;
    double val;
    while (myStream >> val) {
        cdx=0;
        for (; cdx < numCols-1; cdx++) {
            *(Spectrogram+rdx * numCols+cdx) = val;
            myStream >> val;
        };
        *(Spectrogram+rdx * numCols+cdx) = val;
        rdx++;
    }
}

// /////////////////////////////
