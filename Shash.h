//=======================================================================
//=======================================================================

#ifndef _AS_Shash_h
#define _AS_Shash_h

#include "AudioLL.h"

/* =============== spec2shash sub-functions =============== */
/* Driver functions
    to sort the vector elements by third element of tuple
    https://www.geeksforgeeks.org/sorting-vector-tuple-c-ascending-order/
*/
// Ascending
bool sortbythird(const tuple<int,int,int>& a,
    const tuple<int,int,int>& b);
// Descending
bool sortbythird_desc(const tuple<int,int,int>& a,
    const tuple<int,int,int>& b);
// to use heaps to get the n largest peak magnitude value.
struct Comp ;;

/* =============== MAXLIST =============== */
/* MAXLIST function for rows and columns respectively */
void GetPeakCoor1D_row(double * Spectrogram, set<tuple<int,int>> &CoorSetRows,
    int window_size, int rdx, float threshold_abs,
    int num_cols);
void GetPeakCoor1D_col(double * Spectrogram, set<tuple<int,int>> &CoorSetCols,
    int window_size, int cdx, float threshold_abs,
    int num_cols, int num_rows);
/* Speed up : still perform MAXLIST on columns, but each peak is further filtered by looking at its row. */
void GetPeakCoor1D_col_filterrow(double * Spectrogram,
    vector<tuple<int,int,double>> &CoorSetCols,
    int window_size, int cdx, float threshold_abs,
    int num_cols, int num_rows);
/* Speed up : GetPeakCoor1D_col_filterrow, but each peak is further filtered by looking at its 2d neighbourhood. */
void GetPeakCoor1D_col_filter2D(double * Spectrogram,
    vector<tuple<int,int,double>> &CoorSetCols,
    int window_size, int cdx, float threshold_abs,
    int num_cols, int num_rows);
/* Speed up : stillGetPeakCoor1D_col_filter2D but
check row, before check 2d neighbours
To improve : check confirmed candidates coordinates to reduce 2d search time
*/
void GetPeakCoor1D_col_filterrow2D(double * Spectrogram,
    vector<tuple<int,int,double>> &peaks,
    int window_size, int cdx, float threshold_abs,
    int num_cols, int num_rows);


/* =============== get_raw_constellation_pts =============== */
/* Naive O(nm*kk) : Raw extraction of the local max, by 2D windowing */
vector<tuple<int,int,double>> get_raw_constellation_pts_v1(
    int nfft_half, int numFrames, double * x_MAG,
    int t_dim1, int f_dim1,
    int t_dim2=5, int f_dim2=5,
    float threshold_abs=0.0, int base=20
    );
/* O(nm) :
Approx conversion of python's : coordinates1 = skimage.feature.peak_local_max(SPECTROGRAM, min_distance=, threshold_abs=, indices=True)
MAXLIST algo along rows and by columns, then find their intersection.
*/
vector<tuple<int,int,double>> get_raw_constellation_pts_v2(
    int nfft_half, int numFrames, double * Spectrogram,
    int t_dim1, int f_dim1, float threshold_abs, int base
    );
/* Edit on _v2() : Post-process results of GetPeakCoor1D_col_filterrow, instead of running both row & col
    just 1 pass of each col
    then for each selected peak,
    just check whether it is indeed the local max in its column window.
*/
vector<tuple<int,int,double>> get_raw_constellation_pts_v3(
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    );
/* Edit on _v3() : Use GetPeakCoor1D_col_filter2D instead of GetPeakCoor1D_col_filterrow
*/
vector<tuple<int,int,double>> get_raw_constellation_pts_v4(
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    );
/* Edit on _v4() :
    Use GetPeakCoor1D_col_filterrow2D instead of GetPeakCoor1D_col_filterrow
    Pass peak in by reference, instead of creating and copying it out.
*/
void get_raw_constellation_pts_v5(vector<tuple<int,int,double>> &peaks,
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    );

/* =============== get_adjPts =============== */
// This gets the adj points in order, ignoring the magnitude value, then use sort
vector<tuple<int,int,double>> get_adjPts_v1(
    int anchor0, int anchor1,
    vector<tuple<int,int,double>> &peaks,
    int delay_time, int delta_time, int delta_freq_half, int num_tgt_pts
    );
// Replacing sort with heaps
vector<tuple<int,int,double>> get_adjPts_v2(
    int anchor0, int anchor1,
    vector<tuple<int,int,double>> &peaks,
    int delay_time, int delta_time, int delta_freq_half, int num_tgt_pts=5
    );
// Construct heaps from scratch : but it is slower than v2
vector<tuple<int,int,double>> get_adjPts_v3(
    int anchor0,
    int anchor1,
    vector<tuple<int,int,double>> &peaks,
    int delay_time, int delta_time, int delta_freq_half, int num_tgt_pts=5
    );

/* =============== Read/Write =============== */
// Read from file
unordered_map<int, set<int>> txt2hash(const string &fileName) ;
void get_db2bins(
    const string &db_db2bin,
    unordered_map <string, int> &uttDB2bins_DICT) ;

// Write to file
void peaks2txt(const string &fileName, vector<tuple<int,int,double>> &peaks) ;
void hash2txt(const string &fileName, unordered_map<int, set<int>> &map_in) ;

/* =============== spec2shash =============== */
/* Using get_raw_constellation_pts_v2 */
unordered_map <int, set<int>> spec2shash_v1(
    double * x_MAG, int numFrames, int nfft,
    int t_dim1, int f_dim1, float threshold_abs, int num_tgt_pts=5,
    int delay_time=11, int delta_time=50, int delta_freq_half=35
    );
/* Using get_raw_constellation_pts_v3 */
unordered_map <int, set<int>> spec2shash_v2(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts=5,
    int delay_time=11, int delta_time=50, int delta_freq_half=35
    );
/* Using get_raw_constellation_pts_v4 */
unordered_map <int, set<int>> spec2shash_v3(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts=5,
    int delay_time=11, int delta_time=50, int delta_freq_half=35
    );
/* Using get_raw_constellation_pts_v5 */
unordered_map <int, set<int>> spec2shash_v4(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts=5,
    int delay_time=11, int delta_time=50, int delta_freq_half=35
    );
//
/*shashmatching sub-functions*/
/* get_Timeoffsets */
vector<int> get_Timeoffsets_v1(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2) ;
// make_decision (unused for now)
int make_decision_v1(vector<int> Time_offsets) ;
/* Augmented Histogram */
int make_decision_v2(vector<int> Time_offsets) ;

// compute_histogram
double* compute_histogram(double output_Hist[], vector<int> Time_offsets, int bins=100);

/* shashmatching */
// Using make_decision_v2
int shashmatching_v0(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2);
int shashmatching_v1(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2,
    int &num_matches, int threshold_match, int bins);
/* v1 but :
    Ignore if Time_offsets.size() or hash_DICT* < threshold_match
    Using histogram with variable bins
*/
int shashmatching_v2(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2,
    int &num_matches, int threshold_match, int bins);

/*LUT sub-functions*/
// Passing a Look-Up-Table (LUT) in by reference, and adding
void addLUT(
    unordered_map <int, set<string>> &LUT,
    unordered_map <int, set<int>> &hash_DICT_db,
    const string uttid);
// Searching the LUT to get Candidates
/* cand_set is randomly arranged */
set<string> searchLUT_v1(
    unordered_map <int, set<string>> &LUT,
    unordered_map <int, set<int>> &query_hashmap);
/* cand_set is sorted, by the ones with most common occurance */
vector<string> searchLUT_v2(
    unordered_map <int, set<string>> &LUT,
    unordered_map <int, set<int>> &query_hashmap,
    int TopN=2);

// Read from file
unordered_map<int, set<string>> txt2LUT(const string &fileName) ;
// Write to file
void LUT2txt(const string &fileName, unordered_map<int, set<string>> &map_in) ;

#endif // _AS_Shash_h
