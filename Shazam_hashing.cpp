// imports
#include "AudioLL.h"
#include "Shash.h"
using namespace std;

int main (int argc, char ** argv) {

    printf("\n==== Paths =====\n");
    printf("Doing : %s \
        %s %s %s %s \
        %s %s %s %s %s \n", argv[0],
            argv[1], argv[2], argv[3], argv[4],
            argv[5], argv[6], argv[7], argv[8], argv[9]);

    string db_wavscp = argv[1];
    string db_shash_dir = argv[2];
    string db_LUT = argv[3];
    string db_db2bin = argv[4];
    printf("\tdb_wavscp     = %s\n", db_wavscp.c_str());
    printf("\tdb_shash_dir  = %s\n", db_shash_dir.c_str());
    printf("\tdb_LUT        = %s\n", db_LUT.c_str());
    printf("\tdb_db2bin     = %s\n", db_db2bin.c_str());

    int nfft = atoi(argv[5]);
    int MAXLISTWinSize = atoi(argv[6]);   // Window size to perform filtering in choosing peaks
    float threshold_abs = atoi(argv[7]);  // Minmum power spectrum value to be considered as a peak.
    int verbose = atoi(argv[8]);
    int readmode = atoi(argv[9]);
    printf("\n\tnfft=%i\n", nfft);
    printf("\tMAXLISTWinSize=%i\n", MAXLISTWinSize);
    printf("\tthreshold_abs=%f\n", threshold_abs);
    printf("\tverbose=%i\n", verbose);
    printf("\treadmode=%i\n", readmode);


    printf("\n==== Params =====\n");
    // Init (Time)
    time_t start_time_print, end_time_print;
    start_time_print = time(NULL);
    struct timeval start_time_val, end_time_val;
    gettimeofday(&start_time_val, NULL); ios_base::sync_with_stdio(false);
    // Init (Read Audio)
    int AUDIO_DURATION;
    int sample_rate = 8000;
    printf("\tsample_rate=%i\n", sample_rate);
    const int FixedDuration = 60*8000;
    const int SegmentHop = 40*8000;
    int NumberOfSegments, SegmentIdx;
    int TotalSamples, startIdx, endIdx;
    // Init (FFT)
    int hop_length = nfft/2;
    int numFrames = (FixedDuration-nfft)/hop_length + 1;
    printf("\tnfft=%i\n", nfft);
    printf("\thop_length=%i\n", hop_length);
    double windowFunction[nfft];
    computeWindow(nfft, windowFunction, "Hanning");
    double *Spectrogram;
    // Init (shash)
    // int t_dim1 = MAXLISTWinSize;
    // int f_dim1 = MAXLISTWinSize;
    int bins;
    // printf("\tt_dim1=%i\n", t_dim1);
    // printf("\tf_dim1=%i\n", f_dim1);
    unordered_map <int, set<int>> hash_DICT_db;
    unordered_map <int, set<string>> LUT;

    /* initialize all muxers, demuxers and protocols for libavformat
    (does nothing if called twice during the course of one program execution) */
    // av_register_all();

    printf("\n==== Database =====\n");
    // Reading wav.scp
    string uttid, wavpath, curr_line, line, shashpath;
    stringstream shashpath_ss;
    ifstream f_db_wavscp(db_wavscp.c_str()); printf("\n\tStart    reading from = %s \n", db_wavscp.c_str());
    ofstream f_db_db2bin(db_db2bin.c_str()); printf("\n\tStart    writing to   = %s \n", db_db2bin.c_str());
    // complex array to hold fft data
    fftw_complex* fftIn  = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_complex* fftOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_plan p = fftw_plan_dft_1d (nfft, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);
    if (f_db_wavscp.is_open()) {
        while (f_db_wavscp >> uttid >> wavpath ) {
        // Get uttid and string path
            printf("\nuttid=%s \n", uttid.c_str());
            printf("wavpath=%s \n", wavpath.c_str());
            /* ================================================= */
            // Read Audio : Read *.wav at 16kHz, with a downsampler to 8kHz
            vector<short> buffer;
            getlongbuffer(buffer, AUDIO_DURATION, wavpath);
            // verbose
            printf("buffer.size() = %lu = AUDIO_DURATION = %i  AUDIO_DURATION (s) = %f \n", buffer.size(), AUDIO_DURATION, (float) 1.*AUDIO_DURATION/8000);
            /* Split them into 1min chunks */
            TotalSamples = buffer.size();
            NumberOfSegments = 1 + max((TotalSamples-FixedDuration)/SegmentHop, 0);
            for (SegmentIdx=1; SegmentIdx<=NumberOfSegments; SegmentIdx++){
            /* Init the segmentid */
                stringstream segmentid_ss;
                segmentid_ss << uttid << "_" << SegmentIdx;
                string segmentid = segmentid_ss.str();
                if (verbose>0) {
                    printf("\tsegmentid=%s \n", segmentid.c_str());
                }
            /* Crop the audio */
                startIdx = (SegmentIdx-1)*SegmentHop;
                endIdx = min(TotalSamples, startIdx+FixedDuration);
                vector<short> Current_Audio_Segment(buffer.begin() + startIdx, buffer.begin() + endIdx );
            /* Compute FFT */
                Spectrogram = (double *)malloc(numFrames * nfft/2 * sizeof(double));
                computeFullFFT_vector(
                    numFrames, nfft, hop_length, 
                    Current_Audio_Segment, windowFunction, (double*)Spectrogram,
                    (fftw_complex*)fftIn, (fftw_complex*)fftOut, (fftw_plan)p);
            /* Compute Shashing */
                hash_DICT_db = spec2shash_v4(
                    (double*)Spectrogram, numFrames, nfft,
                    MAXLISTWinSize, threshold_abs);
                if (verbose>0) {
                    printf("\thash_DICT_db.size() = %lu \n", hash_DICT_db.size());
                }
                free(Spectrogram);
            /* Write shash to shashpath */
                stringstream shashpath_ss;
                shashpath_ss << db_shash_dir << "/" << segmentid << ".shash";
                shashpath = shashpath_ss.str();
                hash2txt(shashpath, hash_DICT_db);
                if (verbose>0) {
                    printf("\tshashpath=%s \n", shashpath.c_str());
                }
            // Add to LUT
                addLUT(LUT, hash_DICT_db, segmentid);
            // write to db_db2bin
                bins=numFrames/80;
                f_db_db2bin << segmentid << " " << bins << endl;
            }
        }
        // close files
        f_db_wavscp.close();
        f_db_db2bin.flush();
        // destroy fft plan
        fftw_destroy_plan (p);
        fftw_free (fftIn);
        fftw_free (fftOut);
        fftw_cleanup();
    }
    if (verbose>0){
        printf("\n\tFinished reading from = %s \n", db_wavscp.c_str());
        printf("\n\tFinished writing to   = %s \n", db_db2bin.c_str());
    }

    // Write LUT to LUT.txt
    LUT2txt(db_LUT, LUT);
    if (verbose>0)
        printf("\n\tLUT.size() = %lu \n", LUT.size());

    // Time Taken
    end_time_print = time(NULL);
    printf("========================\n");
    printf("Start time  = %s", ctime(&start_time_print));
    printf("End time    = %s", ctime(&end_time_print));
    gettimeofday(&end_time_val, NULL);
    printf("Duration    = %f\n", get_duration(start_time_val, end_time_val));
    printf("========================\n");
    printf("Done\n");
    return 0 ;
}
