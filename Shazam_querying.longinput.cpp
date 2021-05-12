// imports
#include "AudioLL.h"
#include "Shash.h"
using namespace std;
/* ============================================================================================= */
#include "fir_api.h"
#include "typedef.h"
#define FIR_TAP_NUM 15
#define INPUT_SIZE 256
#define OUTPUT_SIZE 128
/* ============================================================================================= */

int main (int argc, char ** argv) {
/* ================================= Paths ===================================================== */
    printf("\n==== Paths =====\n");
    printf("Doing : %s \
        %s %s %s %s \
        %s %s %s %s %s \
        %s %s %s %s \
        %s %s %s \n",
            argv[0], argv[1], argv[2], argv[3], argv[4],
            argv[5], argv[6], argv[7], argv[8], argv[9],
            argv[10], argv[11], argv[12], argv[13],
            argv[14], argv[15], argv[16]);

    string db_wavscp = argv[1];
    string db_shash_dir = argv[2];
    string db_LUT = argv[3];
    string db_db2bin = argv[4];
    printf("\n\tdb_wavscp     = %s\n", db_wavscp.c_str());
    printf(  "\tdb_shash_dir  = %s\n", db_shash_dir.c_str());
    printf(  "\tdb_LUT        = %s\n", db_LUT.c_str());
    printf(  "\tdb_db2bin     = %s\n", db_db2bin.c_str());

    string qry_wavscp = argv[5];
    string qry2db = argv[6];
    string qry_shash_dir = argv[7];
    string qry2nummatch = argv[8];
    string qry2wavpath = argv[9];
    printf("\n\tqry_wavscp    = %s\n", qry_wavscp.c_str());
    printf(  "\tqry2db        = %s\n", qry2db.c_str());
    printf(  "\tqry_shash_dir = %s\n", qry_shash_dir.c_str());
    printf(  "\tqry2nummatch  = %s\n", qry2nummatch.c_str());
    printf(  "\tqry2wavpath   = %s\n", qry2wavpath.c_str());

    int nfft = atoi(argv[10]);
    int MAXLISTWinSize = atoi(argv[11]);    // Window size to perform filtering in choosing peaks
    float threshold_abs = atof(argv[12]);   // Minmum power spectrum value to be considered as a peak.
    int threshold_match = atoi(argv[13]);   // num_matches more than this number is be considered a match
    int TopN = atoi(argv[14]);              // Only search for this number of candidate.
    int min_duration = atoi(argv[15]);      // Audio shorter than this number of seconds is ignored.
    int verbose = atoi(argv[16]);
    printf("\n\tnfft=%i\n", nfft);
    printf("\tMAXLISTWinSize=%i\n", MAXLISTWinSize);
    printf("\tthreshold_abs=%f\n", threshold_abs);
    printf("\tthreshold_match=%i\n", threshold_match);
    printf("\tTopN=%i\n", TopN);
    printf("\tmin_duration=%i\n", min_duration);
    printf("\tverbose=%i\n", verbose);

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
    const int SegmentHop = 20*8000;
    /* For downsampling audio */
    // for FIR
    S16 aswCoeff[FIR_TAP_NUM] = {-1477, 1206, 2225, -233, -3041, 754, 10486, 15894, 10486, 754, -3041, -233, 2225, 1206, -1477 }; // fpass=3400, fstop=4300 @ fs=16000
    S16 aswStateMic1[FIR_TAP_NUM + INPUT_SIZE];
    // Init Downsampler
    FIR_Q15_INSTANCE tFIRMic1;
    FIR_Q15_INIT(&tFIRMic1, FIR_TAP_NUM, aswCoeff, aswStateMic1, INPUT_SIZE);
    // for evaluation
    int TotalSamples, NumberOfHashes;

    /* Init (FFT) */
    int hop_length = nfft/2;
    printf("\thop_length=%i\n", hop_length);
    int numFrames = (FixedDuration-nfft)/hop_length + 1;
    printf("\tnumFrames=%i\n", numFrames);
    double windowFunction[nfft];
    computeWindow(nfft, windowFunction, "Hanning");
    double *Spectrogram;
    int startIdx, endIdx;
    // complex array to hold fft data
    fftw_complex* fftIn  = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_complex* fftOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * nfft);
    fftw_plan p = fftw_plan_dft_1d (nfft, fftIn, fftOut, FFTW_FORWARD, FFTW_ESTIMATE);

    /* Init (Shash params) */
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
    int HashIdx, shash_decision;
    int bins, num_matches;

/* ================================= Database ================================================== */
    printf("\n==== Database =====\n");
    // data/DB/wav.scp -> utt2shash_DICT_DB
    string uttid, numBins, shashpath;
    stringstream shashpath_ss;
    ifstream f_db_db2bin(db_db2bin.c_str());
    printf("\n\tReading from db_db2bin=%s\n", db_db2bin.c_str());
    if (f_db_db2bin.is_open()) {
        while (f_db_db2bin >> uttid >> numBins) {
            // Get uttid and string path
            if (verbose>2){
                printf("\nuttid=%s \n", uttid.c_str());
                printf("\tnumBins=%s \n", numBins.c_str());
            }

            // Read shash from shashpath
            stringstream shashpath_ss;
            shashpath_ss << db_shash_dir << "/" << uttid << ".shash";
            shashpath = shashpath_ss.str();
            hash_DICT_db = txt2hash(shashpath);
            if (verbose>2){
                printf("shashpath=%s \n", shashpath.c_str());
                printf("hash_DICT_db.size() = %lu \n", hash_DICT_db.size());
            }

            // Add to utt2shash_DICT_DB
            utt2shash_DICT_DB[uttid] = hash_DICT_db;
        }
        f_db_db2bin.close();
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
    string utt_QRY, utt_DB, wavpath;
    ifstream f_qry_wavscp(qry_wavscp.c_str());
    ofstream f_qry2db(qry2db.c_str());
    ofstream f_qry2nummatch(qry2nummatch.c_str());
    ofstream f_qry2wavpath(qry2wavpath.c_str());
    printf("\nReading from qry_wavscp=%s\n", qry_wavscp.c_str());
    printf("Writing to   qry2db=%s\n", qry2db.c_str());
    printf("Writing to   qry2nummatch=%s\n", qry2nummatch.c_str());
    printf("Writing to   qry2wavpath=%s\n", qry2wavpath.c_str());
    if (f_qry_wavscp.is_open()) {
        while (f_qry_wavscp >> utt_QRY >> wavpath) {
        /* ================================================= */
            // Get uttid and string path
            if (verbose>0) {
                printf("utt_QRY=%s \n", utt_QRY.c_str());
                if (verbose>1) {
                    printf("wavpath=%s \n", wavpath.c_str());
                }
            }
            stringstream shashpath_ss;
            shashpath_ss << qry_shash_dir << "/" << utt_QRY << ".shash";
            shashpath = shashpath_ss.str();

        /* ================================================= */
            // Read Audio : Read *.wav at 16kHz, with a downsampler to 8kHz
            vector<short> buffer;
            getlongbuffer(buffer, AUDIO_DURATION, wavpath);
            // verbose
            printf("\tbuffer.size() = %lu = AUDIO_DURATION = %i  AUDIO_DURATION (s) = %f \n", buffer.size(), AUDIO_DURATION, (float) 1.*AUDIO_DURATION/8000);
            // Ignore if : less than MinDuration seccond of audio, (1s ~= 2 windows of the MAXLISTWinSize)
            if (AUDIO_DURATION < MinDuration) {
                printf("\tAUDIO_DURATION=%i<%i : ignoring...\n", AUDIO_DURATION, MinDuration);
                continue;
            }
            /* Split them into 1min chunks */
            TotalSamples = buffer.size();
            NumberOfHashes = 1 + max((TotalSamples-FixedDuration)/SegmentHop, 0);
            for (HashIdx=1; HashIdx<=NumberOfHashes; HashIdx++){
            /* Crop the audio */
                startIdx = (HashIdx-1)*SegmentHop;
                endIdx = min(TotalSamples, startIdx+FixedDuration);
                vector<short> Current_Audio_Segment(buffer.begin() + startIdx, buffer.begin() + endIdx );
            /* Compute FFT */
                Spectrogram = (double *)malloc(numFrames * nfft/2 * sizeof(double));
                computeFullFFT_vector(
                    numFrames, nfft, hop_length, 
                    Current_Audio_Segment, windowFunction, (double*)Spectrogram,
                    (fftw_complex*)fftIn, (fftw_complex*)fftOut, (fftw_plan)p);
            /* Compute Shashing */
                unordered_map <int, set<int>> hash_DICT_qry = spec2shash_v4(
                    (double*)Spectrogram, numFrames, nfft,
                    MAXLISTWinSize, threshold_abs);
                // out_shashes.push_back(hash_DICT_qry);
                free(Spectrogram);
            /* Shashing Matching */
                CandidateSet = searchLUT_v2(LUT, hash_DICT_qry, TopN);
                // printf("\tHashIdx = %i : Current_Audio_Segment length = %lu | hash_DICT_qry = %lu | CandidateSet = %lu \n", 
                //     HashIdx, Current_Audio_Segment.size(), 
                //     hash_DICT_qry.size(), CandidateSet.size());
                shash_decision = 0;
                for (const auto &utt_DB : CandidateSet){

                    // Extract the candidate db hash
                    hash_DICT_db = utt2shash_DICT_DB[utt_DB];
                    bins = uttDB2bins_DICT[utt_DB];

                    // Perform matching
                    shash_decision = shashmatching_v2(hash_DICT_db, hash_DICT_qry, num_matches, threshold_match, bins);
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
                if (shash_decision==1) {
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

/* ================================= Time Taken ===================================================== */
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
