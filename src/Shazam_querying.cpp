// imports
#include "Shash.h"
using namespace std;
/* ============================================================================================= */
#include "fir_api.h"
#include "typedef.h"
#define FIR_TAP_NUM 15
#define INPUT_SIZE 256
#define OUTPUT_SIZE 128
/* ============================================================================================= */

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
DEFINE_double(threshold_abs, 8192, "Threshold for hashing (thabs_db=1, thabs_qry=8192)");

/* -- Leonard's parameter for querying -- */
DEFINE_int32(threshold_match, 110, "num_matches more than this number is be considered a match");
DEFINE_int32(TopN, 2, "Early stop criteria when doing matching");
DEFINE_int32(min_duration, 5, "Audio shorter than this number of seconds is ignored.");

/* -- Input db -- */
DEFINE_string(db_wavscp,    "./wav.scp",    "db_wavscp");
DEFINE_string(db_shash_dir, "./shash",      "db_shash_dir");
DEFINE_string(db_LUT,       "./LUT",        "db_LUT");
DEFINE_string(db_db2bin,    "./db2bin",     "db_db2bin");
/* -- Input qry -- */
DEFINE_string(qry_wavscp,       "./data/*/wav.scp",         "qry_wavscp");
/* -- Output qry -- */
DEFINE_string(qry2db,           "./data/*/*/qry2db",        "qry2db");
DEFINE_string(qry2nummatch,     "./data/*/*/qry2nummatch",  "qry2nummatch");
DEFINE_string(qry2wavpath,      "./data/*/*/qry2wavpath",   "qry2wavpath");
DEFINE_string(qry_shash_dir,    "./shash/*",          "qry_shash_dir");

int main (int argc, char ** argv) {

    // if (argc == 1) {
    if (argc < 10) {
        // report version
        cout << "Usage: " << endl << argv[0] 
        << " -db_wavscp" 
        << " -db_shash_dir" 
        << " -db_LUT" 
        << " -db_db2bin" 
            << " -qry_wavscp" 
            << " -qry2db" 
            << " -qry2nummatch" 
            << " -qry2wavpath" 
            << " -qry_shash_dir" 
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
    printf("\n");
    printf("FLAGS_qry_wavscp        : %s\n", FLAGS_qry_wavscp.c_str());
    printf("FLAGS_qry2db            : %s\n", FLAGS_qry2db.c_str());
    printf("FLAGS_qry2nummatch      : %s\n", FLAGS_qry2nummatch.c_str());
    printf("FLAGS_qry2wavpath       : %s\n", FLAGS_qry2wavpath.c_str());
    printf("FLAGS_qry_shash_dir     : %s\n", FLAGS_qry_shash_dir.c_str());

    /* -- db paths -- */
    string db_wavscp = FLAGS_db_wavscp;
    string db_shash_dir = FLAGS_db_shash_dir;
    string db_LUT = FLAGS_db_LUT;
    string db_db2bin = FLAGS_db_db2bin;
    printf("\n\tdb_wavscp     = %s\n", db_wavscp.c_str());
    printf(  "\tdb_shash_dir  = %s\n", db_shash_dir.c_str());
    printf(  "\tdb_LUT        = %s\n", db_LUT.c_str());
    printf(  "\tdb_db2bin     = %s\n", db_db2bin.c_str());

    /* -- qry paths -- */
    string qry_wavscp = FLAGS_qry_wavscp;
    string qry2db = FLAGS_qry2db;
    string qry_shash_dir = FLAGS_qry_shash_dir;
    string qry2nummatch = FLAGS_qry2nummatch;
    string qry2wavpath = FLAGS_qry2wavpath;
    printf("\n\tqry_wavscp    = %s\n", qry_wavscp.c_str());
    printf(  "\tqry2db        = %s\n", qry2db.c_str());
    printf(  "\tqry_shash_dir = %s\n", qry_shash_dir.c_str());
    printf(  "\tqry2nummatch  = %s\n", qry2nummatch.c_str());
    printf(  "\tqry2wavpath   = %s\n", qry2wavpath.c_str());

    /* -- query params -- */
    int threshold_match = FLAGS_threshold_match;   // num_matches more than this number is be considered a match
    int TopN = FLAGS_TopN;              // Only search for this number of candidate.
    int min_duration = FLAGS_min_duration;      // Audio shorter than this number of seconds is ignored.

/* ================================= Params ==================================================== */
    printf("\n==== Params =====\n");
    /* Init (Time) */
    time_t start_time_print, end_time_print;
    start_time_print = time(NULL);
    struct timeval start_time_val, end_time_val;
    gettimeofday(&start_time_val, NULL); ios_base::sync_with_stdio(false);

    /* Init (Read Audio) */
    int AUDIO_DURATION;
    const int FixedDuration = 20*8000;
    const int MinDuration = min_duration*8000;
    /* For downsampling audio */
    // for FIR
    S16 aswCoeff[FIR_TAP_NUM] = {-1477, 1206, 2225, -233, -3041, 754, 10486, 15894, 10486, 754, -3041, -233, 2225, 1206, -1477 }; // fpass=3400, fstop=4300 @ fs=16000
    S16 aswStateMic1[FIR_TAP_NUM + INPUT_SIZE];
    // for evaluation
    FILE* pfMic;
    short aswMic1[INPUT_SIZE];
    short aswMic1_Out[INPUT_SIZE];
    // S16* buffer_8k;
    S16 buffer_8k[FixedDuration];
    S16 ShortZero=0;
    // Init Downsampler
    FIR_Q15_INSTANCE tFIRMic1;
    FIR_Q15_INIT(&tFIRMic1, FIR_TAP_NUM, aswCoeff, aswStateMic1, INPUT_SIZE);

    /* Init (FFT) */
    int nfft = FLAGS_nfft;
    int hop_length = nfft/2;
    printf("\tnfft=%i\n", nfft);
    printf("\thop_length=%i\n", hop_length);
    int numFrames = (FixedDuration-nfft)/hop_length + 1;
    printf("\tnumFrames=%i\n", numFrames);
    double windowFunction[nfft];
    computeWindow(nfft, windowFunction, "Hanning");
    double *Spectrogram;

    /* Init (Shash params) */
    float threshold_abs = FLAGS_threshold_abs;  // Minmum power spectrum value to be considered as a peak.
    int MAXLISTWinSize = FLAGS_MAXLISTWinSize;   // Window size to perform filtering in choosing peaks
    int t_dim1 = MAXLISTWinSize; // filter size (temporal)
    int f_dim1 = MAXLISTWinSize; // filter size (frequency)
    printf("\tt_dim1=%i\n", t_dim1);
    printf("\tf_dim1=%i\n", f_dim1);
    // Init (shash for db)
    unordered_map <int, set<int>> hash_DICT_db;
    unordered_map <int, set<string>> LUT;
    unordered_map <string, unordered_map <int,set<int>>> utt2shash_DICT_DB;
    unordered_map <string, int> uttDB2bins_DICT;
    // Init (shash for qry)
    unordered_map <int, set<int>> hash_DICT_qry;
    vector<string> CandidateSet;
    int bins, num_matches;

/* ================================= Database ================================================== */
    printf("\n==== Database =====\n");
    // data/DB/wav.scp -> utt2shash_DICT_DB
    string uttid, wavpath, shashpath;
    stringstream shashpath_ss;
    ifstream f_db_wavscp(db_wavscp.c_str());
    printf("\n\tReading from db_wavscp=%s\n", db_wavscp.c_str());
    if (f_db_wavscp.is_open()) {
        while (f_db_wavscp >> uttid >> wavpath) {
            // Get uttid and string path
            printf("\nuttid=%s \n", uttid.c_str());
            printf("\twavpath=%s \n", wavpath.c_str());

            // Read shash from shashpath
            stringstream shashpath_ss;
            shashpath_ss << db_shash_dir << "/" << uttid << ".shash";
            shashpath = shashpath_ss.str();
            hash_DICT_db = txt2hash(shashpath);
            printf("shashpath=%s \n", shashpath.c_str());
            printf("hash_DICT_db.size() = %lu \n", hash_DICT_db.size());

            // Add to utt2shash_DICT_DB
            utt2shash_DICT_DB[uttid] = hash_DICT_db;
        }
        f_db_wavscp.close();
    }
    printf("\n\tutt2shash_DICT_DB.size() = %lu \n", utt2shash_DICT_DB.size());

    // Read from LUT
    printf("\n\tReading from LUT\n");
    LUT = txt2LUT(db_LUT);
    printf("\tLUT.size() = %lu \n", LUT.size());

    // Read from db2bin
    printf("\n\tReading from db2bin\n");
    get_db2bins(db_db2bin, uttDB2bins_DICT);

/* ================================= Query ===================================================== */
    printf("\n==== Query =====\n");
    // data/QRY/wav.scp -> data/QRY/qry2db
    string utt_QRY, utt_DB;
    ifstream f_qry_wavscp(qry_wavscp.c_str());
    ofstream f_qry2db(qry2db.c_str());
    ofstream f_qry2nummatch(qry2nummatch.c_str());
    ofstream f_qry2wavpath(qry2wavpath.c_str());
    printf("\nReading from qry_wavscp=%s\n", qry_wavscp.c_str());
    printf("Writing to   qry2db=%s\n", qry2db.c_str());
    printf("Writing to   qry2nummatch=%s\n", qry2nummatch.c_str());
    printf("Writing to   qry2wavpath=%s\n", qry2wavpath.c_str());
    // complex array to hold fft data
    fftw_complex* fftIn  = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_complex* fftOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_plan p = fftw_plan_dft_1d (nfft, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);
    if (f_qry_wavscp.is_open()) {
        while (f_qry_wavscp >> utt_QRY >> wavpath) {
        /* ================================================= */
            // Get uttid and string path
            printf("utt_QRY=%s \n", utt_QRY.c_str());
            printf("wavpath=%s \n", wavpath.c_str());
            stringstream shashpath_ss;
            shashpath_ss << qry_shash_dir << "/" << utt_QRY << ".shash";
            shashpath = shashpath_ss.str();

        /* ================================================= */
            // Read Audio : Read *.wav at 16kHz, with a downsampler to 8kHz
            // Get Audio information
            if ((pfMic = fopen(wavpath.c_str(), "rb")) == NULL) {
                printf ("\tError : Not able to open input file '%s'\n", wavpath.c_str());
                fclose(pfMic);
                continue;
            }
            // buffer_8k = (S16 *)calloc(FixedDuration, sizeof(S16));
            // memset(buffer_8k, 0, sizeof(buffer_8k));
            fill(buffer_8k, buffer_8k+(FixedDuration), ShortZero);
            AUDIO_DURATION=0;
            while (1) {

                // read chunk
                if (fread(aswMic1, sizeof(S16), (INPUT_SIZE), pfMic) != (INPUT_SIZE)) break;

                // filtering
                FIR_Q15(&tFIRMic1, (S16 *)aswMic1, (S16 *)aswMic1_Out, INPUT_SIZE);
                // decimation
                for (unsigned i1 = 0; i1 < OUTPUT_SIZE; i1++)
                    buffer_8k[AUDIO_DURATION+i1] = aswMic1_Out[2 * i1];
                AUDIO_DURATION += OUTPUT_SIZE;

                // exit if more than 20s
                if (AUDIO_DURATION>(FixedDuration-OUTPUT_SIZE)){
                    break;
                }
            }
            fclose(pfMic);
            // verbose
            printf("\tAUDIO_DURATION=%i, Seconds=%i\n", AUDIO_DURATION, AUDIO_DURATION/8000);
            // Ignore if : less than MinDuration seccond of audio, (1s ~= 2 windows of the MAXLISTWinSize)
            printf("\tAUDIO_DURATION=%i<%i : ignoring...\n", AUDIO_DURATION, MinDuration);

        /* ================================================= */
            // Compute FFT
            Spectrogram = (double *)calloc((numFrames * hop_length), sizeof(double));
            // computeFullFFT_short( numFrames, nfft, hop_length, (S16*)buffer_8k, windowFunction, (double*)Spectrogram);
            computeFullFFT_short_fast(
                numFrames, nfft, hop_length,
                (S16*)buffer_8k, windowFunction, (double*)Spectrogram,
                (fftw_complex*)fftIn, (fftw_complex*)fftOut, (fftw_plan)p);

        /* ================================================= */
            // Get shash
            hash_DICT_qry = spec2shash_v4((double*)Spectrogram, numFrames, nfft, f_dim1, threshold_abs);
            // Free variables
            free(Spectrogram);
            // free(buffer_8k);

            if (hash_DICT_qry.size()<threshold_match){
                printf("\t(hash_DICT_qry=%lu) < (%i=threshold_match). \n", hash_DICT_qry.size(), threshold_match);
                continue;
            }
            printf("\thash_DICT_qry.size() = %lu \n", hash_DICT_qry.size());

        /* ================================================= */
            // shashmatching
            CandidateSet = searchLUT_v2(LUT, hash_DICT_qry, TopN);
            int shash_decision = 0;
            // for (auto curr_utt_DB=CandidateSet.begin(); curr_utt_DB != CandidateSet.end(); ++curr_utt_DB) {
            for (const auto &utt_DB : CandidateSet){

                // Extract the candidate db hash
                hash_DICT_db = utt2shash_DICT_DB[utt_DB];
                bins = uttDB2bins_DICT[utt_DB];

                // Perform matching
                shash_decision = shashmatching_v2(hash_DICT_db, hash_DICT_qry, num_matches, threshold_match, bins);
                printf("\tnum_matches = %i \tutt_DB = %s\n", num_matches, (utt_DB).c_str());
                printf("\tshash_decision = %i \n", shash_decision);
                printf("\tbins = %i \n", bins);
                if (num_matches>0) {
                    f_qry2nummatch << utt_QRY << " " << num_matches << " " << utt_DB << endl;
                }

                // Found a match
                if (shash_decision==1) {
                    hash2txt(shashpath, hash_DICT_qry);
                    printf("\t%s matched with %s , num_matches=%i\n", utt_QRY.c_str(), utt_DB.c_str(), num_matches);
                    f_qry2db << utt_QRY << " " << utt_DB << " " << num_matches << endl;
                    f_qry2wavpath << utt_QRY << " " << wavpath << endl;
                    break;
                }
            }
        }
        // close files
        f_qry_wavscp.close();
        f_qry2db.flush();
        f_qry2nummatch.flush();
        f_qry2wavpath.flush();
        printf("Written to %s \n", qry2db.c_str());
        printf("Written to %s \n", qry2nummatch.c_str());
        printf("Written to %s \n", qry2wavpath.c_str());
        // destroy fft plan
        fftw_destroy_plan (p);
        fftw_free (fftIn);
        fftw_free (fftOut);
    }

    // Time Taken
    end_time_print = time(NULL);
    printf("========================\n");
    printf("Start time  = %s", ctime(&start_time_print));
    printf("End time    = %s", ctime(&end_time_print));
    gettimeofday(&end_time_val, NULL);
    printf("Duration    = %f\n", get_duration(start_time_val, end_time_val));
    printf("========================\n");
    printf("Done\n");
string ScriptName = argv[0];
printf("%s done\n", ScriptName.c_str());
return 0 ;
}
