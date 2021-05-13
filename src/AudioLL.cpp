/* -------------------------------------- imports -------------------------------------- */
#include "AudioLL.h"
extern "C"
{
    #include <libavutil/opt.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

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
void read_audio_double(std::string filePath_str, int AUDIO_DURATION, double * output_buffer){
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
void GetAudioInformation(std::string filePath_str, int &AUDIO_DURATION, int &numFrames, int nfft, int hop_length){
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


/* -------------------------------------- FFT -------------------------------------- */
void computeWindow(int nfft, double windowFunction[], std::string windowType){
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
        std::fill_n(windowFunction, nfft, 1);
    } else {
        printf("\t%s() : No such window exists\n", __func__);
        throw;
    }
    printf("\t%s() : Using %s window\n", __func__, windowType.c_str());
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
    std::vector<float>& buffer, std::vector<float>& windowFunction, double * Spectrogram,
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
void wave2txt(const std::string &fileName, signed short * buffer, int AUDIO_DURATION) {
    printf("%s() : fileName=%s\n", __func__, fileName.c_str());
    printf("\t%s() : AUDIO_DURATION=%i\n", __func__, AUDIO_DURATION);
    std::ofstream myFile(fileName);
    if (!myFile.is_open()) {
        printf("\t%s() : Failed to open file\n", __func__);
        return;
    }
    printf("\t%s() : Writing to : %s\n", __func__, fileName.c_str());

    for (int idx=0; idx<AUDIO_DURATION; ++idx){
        myFile << *(buffer+idx) << " ";
    }
    myFile << std::endl;
    myFile.flush();
    printf("\t%s() : Written to : %s\n", __func__, fileName.c_str());
}
void spec2txt(const std::string &fileName, double * Spectrogram, int numFrames, int nfft) {
    printf("%s() : fileName=%s\n", __func__, fileName.c_str());
    printf("%s() : numFrames=%i\n", __func__, numFrames);
    printf("%s() : nfft=%i\n", __func__, nfft);
    std::ofstream myFile(fileName);
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
        myFile << std::endl;
    }
    myFile.flush();
    printf("%s() : Written to : %s\n", __func__, fileName.c_str());
}
/* -- Read from file -- */
void txt2spec(const std::string &fileName, double * Spectrogram, int numCols) {
    std::ifstream myStream(fileName);
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
