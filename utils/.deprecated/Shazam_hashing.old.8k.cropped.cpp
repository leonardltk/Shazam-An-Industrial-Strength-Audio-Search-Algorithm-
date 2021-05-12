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
    SNDFILE *infile;
    SF_INFO sfinfo;
    memset (&sfinfo, 0, sizeof (sfinfo));
    double* buffer;
    int sample_rate = 8000;
    printf("\tsample_rate=%i\n", sample_rate);
    // Init (FFT)
    int hop_length = nfft/2;
    int numFrames;
    printf("\tnfft=%i\n", nfft);
    printf("\thop_length=%i\n", hop_length);
    double windowFunction[nfft];
    computeWindow(nfft, windowFunction, "Hanning");
    // Init (shash)
    int t_dim1 = MAXLISTWinSize;
    int f_dim1 = MAXLISTWinSize;
    int bins;
    printf("\tt_dim1=%i\n", t_dim1);
    printf("\tf_dim1=%i\n", f_dim1);
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
            if (verbose>0){
                printf("\tuttid=%s \n", uttid.c_str());
                printf("\twavpath=%s \n", wavpath.c_str());
            }

        // Read audio to buffer
            if (readmode==1){    // Read mp3
                if (read_audio_mp3(wavpath, sample_rate, &buffer, AUDIO_DURATION, numFrames, nfft, hop_length) != 0) {
                    printf("Cannot read %s\n", wavpath.c_str());
                    continue;
                }
            } else if (readmode==2){    // Read wav (without dup buffer)
                if ((infile = sf_open (wavpath.c_str(), SFM_READ, &sfinfo)) == NULL)     {
                    printf ("\tError : Not able to open input file '%s'\n", wavpath.c_str());
                    sf_close (infile);
                    continue;
                }
                AUDIO_DURATION = sfinfo.frames;
                AUDIO_DURATION -= (AUDIO_DURATION-nfft) % hop_length; // Truncate
                numFrames = (AUDIO_DURATION-nfft)/hop_length + 1;

                buffer = (double *)malloc(AUDIO_DURATION * sizeof(double));
                sf_read_double (infile, buffer, AUDIO_DURATION);
                sf_close (infile) ;
            } else if (readmode==3){
                throw;
            }
            bins=numFrames/80;
            if (verbose>0) {
                printf("\tAUDIO_DURATION = %i\n", AUDIO_DURATION);
                printf("\tnumFrames = %i\n", numFrames);
                printf("\tbins = %i\n", bins);
            }

        // Compute FFT
            double *Spectrogram = (double *)malloc(numFrames * nfft/2 * sizeof(double));
            // computeFullFFT_double(numFrames, nfft, hop_length, (double*)buffer, windowFunction, (double*)Spectrogram);
            computeFullFFT_double_fast(
                numFrames, nfft, hop_length,
                (double*)buffer, windowFunction, (double*)Spectrogram,
                (fftw_complex*)fftIn, (fftw_complex*)fftOut, (fftw_plan)p);

        // Get shash
            hash_DICT_db = spec2shash_v4(
                (double*)Spectrogram, numFrames, nfft,
                f_dim1, threshold_abs);
            if (verbose>0)
                printf("\thash_DICT_db.size() = %lu \n", hash_DICT_db.size());

        // Free variables
            free(buffer);
            free(Spectrogram);

        // Write shash to shashpath
            stringstream shashpath_ss;
            shashpath_ss << db_shash_dir << "/" << uttid << ".shash";
            shashpath = shashpath_ss.str();
            hash2txt(shashpath, hash_DICT_db);
            if (verbose>0)
                printf("\tshashpath=%s \n", shashpath.c_str());

        // Add to LUT
            addLUT(LUT, hash_DICT_db, uttid);

        // write to db_db2bin
            f_db_db2bin << uttid << " " << bins << endl;
        }
        // close files
        f_db_wavscp.close();
        f_db_db2bin.flush();
        // destroy fft plan
        fftw_destroy_plan (p);
        fftw_free (fftIn);
        fftw_free (fftOut);
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
