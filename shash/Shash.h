//=======================================================================
//=======================================================================

#ifndef _AS_Shash_h
#define _AS_Shash_h

// #include "AudioLL.h"
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
// Duration
#include <bits/stdc++.h>
#include <sys/time.h>
#include <stack>
#include <ctime>
#include <unistd.h>
// Misc
#include <numeric>

/* =============== spec2shash sub-functions =============== */
/* to use heaps to get the n largest peak magnitude value. */
struct Comp ;

/* =============== Get Peaks =============== */
void GetPeaks(double * Spectrogram,
    std::vector<std::tuple<int,int,double>> &peaks,
    int window_size, int cdx, float threshold_abs,
    int num_cols, int num_rows);

/* =============== get_raw_constellation_pts =============== */
void get_raw_constellation_pts(std::vector<std::tuple<int,int,double>> &peaks,
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base);

/* =============== get_adjPts =============== */
std::vector<std::tuple<int,int,double>> get_adjPts(
    int anchor0, int anchor1,
    std::vector<std::tuple<int,int,double>> &peaks,
    int delay_time, int delta_time, int delta_freq_half, int num_tgt_pts=5);

/* =============== spec2shash =============== */
/* Using get_raw_constellation_pts */
std::unordered_map <int, std::set<int>> spec2shash_v4(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts=5,
    int delay_time=11, int delta_time=50, int delta_freq_half=35);

/*shashmatching sub-functions*/
std::vector<int> get_Timeoffsets(
    std::unordered_map <int, std::set<int>> &hash_DICT1,
    std::unordered_map <int, std::set<int>> &hash_DICT2); 
// compute_histogram
double* compute_histogram(double output_Hist[], std::vector<int> Time_offsets, int bins);

/* shashmatching */
int shashmatching_v2(
    std::unordered_map <int, std::set<int>> &hash_DICT1,
    std::unordered_map <int, std::set<int>> &hash_DICT2,
    int &num_matches, int threshold_match, int bins);

/*LUT sub-functions*/
// Passing a Look-Up-Table (LUT); in by reference, and adding
void addLUT(
    std::unordered_map <int, std::set<size_t>> &LUT,
    std::unordered_map <int, std::set<int>> &hash_DICT_db,
    size_t uttid);
void eraseLUT(
    std::unordered_map <int, std::set<size_t>> &LUT,
    std::unordered_map <int, std::set<int>> &hash_DICT_db,
    size_t uttid);
// Searching the LUT to get Candidates
/* cand_set is sorted, by the ones with most common occurance */
std::vector<size_t> searchLUT_v2(
    std::unordered_map <int, std::set<size_t>> &LUT,
    std::unordered_map <int, std::set<int>> &query_hashmap,
    int TopN=2);

#endif // _AS_Shash_h
