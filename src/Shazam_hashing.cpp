// imports
#include "Shash.h"
using namespace std;

/* -- Deployment tools -- */
DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, 300, "Connection will be closed if there is no read/write operations during the last `idle_timeout_s'");
DEFINE_bool(use_auto_limiter, false, "");
DEFINE_string(service_name, "hello_shash", "Original's was charlie's likee-audio-audit");
DEFINE_int32(num_decoder, 32, "");
DEFINE_int32(num_hasher, 32, "");
DEFINE_bool(log_request, false, "whether dump request attachment.");
BRPC_VALIDATE_GFLAG(log_request, brpc::PassValidate);

/* -- Leonard's parameter for hashing -- */
DEFINE_int32(sample_rate, 8000, "");
DEFINE_int32(nfft, 512, "");
DEFINE_int32(MAXLISTWinSize, 7, "window length to search for peaks during hashing");
DEFINE_double(threshold_abs, 1, "Threshold for hashing (thabs_db=1, thabs_qry=8192)");

/* -- Input db -- */
DEFINE_string(db_wavscp,    "./wav.scp",    "db_wavscp");
/* -- Output db -- */
DEFINE_string(db_shash_dir, "./shash",      "db_shash_dir");
DEFINE_string(db_LUT,       "./LUT",        "db_LUT");
DEFINE_string(db_db2bin,    "./db2bin",     "db_db2bin");

/* -- main -- */
int main(int argc, char** argv){

    // if (argc == 1) {
    if (argc < 5) {
        // report version
        cout << "Usage: " << endl << argv[0]
        << " -db_wavscp" 
        << " -db_shash_dir" 
        << " -db_LUT" 
        << " -db_db2bin" 
        << endl;
        return 1;
    }

    /* GFLAGS */
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    printf("\n");
    printf("FLAGS_port              : %i\n", FLAGS_port);
    printf("FLAGS_idle_timeout_s    : %i\n", FLAGS_idle_timeout_s);
    printf("FLAGS_use_auto_limiter  : %s\n", FLAGS_use_auto_limiter ? "true" : "false");
    printf("FLAGS_service_name      : %s\n", FLAGS_service_name.c_str());
    printf("FLAGS_num_decoder       : %i\n", FLAGS_num_decoder);
    printf("FLAGS_num_hasher        : %i\n", FLAGS_num_hasher);
    printf("FLAGS_log_request       : %s\n", FLAGS_log_request ? "true" : "false");
    printf("\n");
    printf("FLAGS_sample_rate       : %i\n", FLAGS_sample_rate);
    printf("FLAGS_nfft              : %i\n", FLAGS_nfft);
    printf("FLAGS_MAXLISTWinSize    : %i\n", FLAGS_MAXLISTWinSize);
    printf("FLAGS_threshold_abs     : %f\n", FLAGS_threshold_abs);
    printf("\n");
    printf("FLAGS_db_wavscp         : %s\n", FLAGS_db_wavscp.c_str());
    printf("FLAGS_db_shash_dir      : %s\n", FLAGS_db_shash_dir.c_str());
    printf("FLAGS_db_LUT            : %s\n", FLAGS_db_LUT.c_str());
    printf("FLAGS_db_db2bin         : %s\n", FLAGS_db_db2bin.c_str());

    printf("\n==== Params =====\n");
    string db_wavscp = FLAGS_db_wavscp;
    string db_shash_dir = FLAGS_db_shash_dir;
    string db_LUT = FLAGS_db_LUT;
    string db_db2bin = FLAGS_db_db2bin;
    printf("\tdb_wavscp     = %s\n", db_wavscp.c_str());
    printf("\tdb_shash_dir  = %s\n", db_shash_dir.c_str());
    printf("\tdb_LUT        = %s\n", db_LUT.c_str());
    printf("\tdb_db2bin     = %s\n", db_db2bin.c_str());
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
    int sample_rate = FLAGS_sample_rate;
    printf("\tsample_rate=%i\n", sample_rate);
    // Init (FFT)
    int nfft = FLAGS_nfft;
    int hop_length = nfft/2;
    int numFrames;
    printf("\tnfft=%i\n", nfft);
    printf("\thop_length=%i\n", hop_length);
    double windowFunction[nfft];
    computeWindow(nfft, windowFunction, "Hanning");
    // Init (shash)
    float threshold_abs = FLAGS_threshold_abs;  // Minmum power spectrum value to be considered as a peak.
    int MAXLISTWinSize = FLAGS_MAXLISTWinSize;   // Window size to perform filtering in choosing peaks
    int t_dim1 = MAXLISTWinSize;
    int f_dim1 = MAXLISTWinSize;
    int bins;
    printf("\tthreshold_abs=%f\n", threshold_abs);
    printf("\tMAXLISTWinSize=%i\n", MAXLISTWinSize);
    printf("\tt_dim1=%i\n", t_dim1);
    printf("\tf_dim1=%i\n", f_dim1);
    unordered_map <int, set<string>> LUT;
    unordered_map <int, set<int>> hash_DICT_db;

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
        /* Display uttid and string path */
            printf("uttid       : %s\n", uttid.c_str());
            printf("wavpath     : %s\n", wavpath.c_str());

        /* Read audio to buffer */
            if ((infile = sf_open (wavpath.c_str(), SFM_READ, &sfinfo)) == NULL)     {
                printf ("\tError : Not able to open input file '%s'\n", wavpath.c_str());
                sf_close (infile);
                throw;
            }
            AUDIO_DURATION = sfinfo.frames;
            AUDIO_DURATION -= (AUDIO_DURATION-nfft) % hop_length; // Truncate
            numFrames = (AUDIO_DURATION-nfft)/hop_length + 1;
            bins=numFrames/80;

            buffer = (double *)malloc(AUDIO_DURATION * sizeof(double));
            sf_read_double (infile, buffer, AUDIO_DURATION);
            sf_close (infile) ;

            printf("\tAUDIO_DURATION = %i\n", AUDIO_DURATION);
            printf("\tnumFrames = %i\n", numFrames);
            printf("\tbins = %i\n", bins);

        /* Compute FFT */
            double *Spectrogram = (double *)malloc(numFrames * nfft/2 * sizeof(double));
            // computeFullFFT_double(numFrames, nfft, hop_length, (double*)buffer, windowFunction, (double*)Spectrogram);
            computeFullFFT_double_fast(
                numFrames, nfft, hop_length,
                (double*)buffer, windowFunction, (double*)Spectrogram,
                (fftw_complex*)fftIn, (fftw_complex*)fftOut, (fftw_plan)p);

        /* Get shash */
            hash_DICT_db = spec2shash_v4(
                (double*)Spectrogram, numFrames, nfft,
                f_dim1, threshold_abs);
            printf("\thash_DICT_db.size() = %lu \n", hash_DICT_db.size());

        /* Free variables */
            free(buffer);
            free(Spectrogram);

        /* Write shash to shashpath */
            stringstream shashpath_ss;
            shashpath_ss << db_shash_dir << "/" << uttid << ".shash";
            shashpath = shashpath_ss.str();
            hash2txt(shashpath, hash_DICT_db);
            printf("shashpath   : %s\n", shashpath.c_str());

        /* Add to LUT */
            addLUT(LUT, hash_DICT_db, uttid);

        /* write to db_db2bin */
            f_db_db2bin << uttid << " " << bins << endl;
        }
        /* close files */
        f_db_wavscp.close();
        f_db_db2bin.flush();
        /* destroy fft plan */
        fftw_destroy_plan (p);
        fftw_free (fftIn);
        fftw_free (fftOut);
    }
    printf("\n\tFinished reading from = %s \n", db_wavscp.c_str());
    printf("\n\tFinished writing to   = %s \n", db_db2bin.c_str());

    // Write LUT to LUT.txt
    LUT2txt(db_LUT, LUT);
    printf("\n\tLUT.size() = %lu \n", LUT.size());

    // Time Taken
    end_time_print = time(NULL);
    printf("========================\n");
    string ScriptName = argv[0];
    printf("%s done\n", ScriptName.c_str());
    printf("Start time  = %s", ctime(&start_time_print));
    printf("End time    = %s", ctime(&end_time_print));
    gettimeofday(&end_time_val, NULL);
    printf("Duration    = %f\n", get_duration(start_time_val, end_time_val));
    printf("========================\n");
    printf("Done\n");
    return 0 ;
}
