/* -------------------------------------- imports -------------------------------------- */
#include "Shash.h"

/* =============== spec2shash sub-functions =============== */
/* to use heaps to get the n largest peak magnitude value. */
struct Comp {

    // // min heap
    // bool operator()(const std::tuple<int,int,int>& a,
    //         const std::tuple<int,int,int>& b){
    //        return (std::get<2>(a) > std::get<2>(b));
    // }

    // // max heap
    bool operator()(const std::tuple<int,int,int>& a,
        const std::tuple<int,int,int>& b){
        return (std::get<2>(a) < std::get<2>(b));
    }
};

/* =============== Get Peaks =============== */
void GetPeaks(double * Spectrogram,
    std::vector<std::tuple<int,int,double>> &peaks,
    int window_size, int cdx, float threshold_abs,
    int num_cols, int num_rows){
    /* Implementing the MAXLIST algorithms to get local max*/

    // Init
    /*
        Create a Double Ended Queue, Qi that will store indexes of Spectrogram elements
        The queue will store indexes of useful elements in current window and it will
        maintain decreasing order of values from front to rear in Qi,
        i.e., Spectrogram[rdx][Qi.front[]] to Spectrogram[rdx][Qi.rear()] are sorted in decreasing order
    */
    std::deque<int> Qi(window_size);
    /*
        Storage of the running index of the LatestMaxIdx, to compare with the NextMaxIdx.
        If NextMaxIdx has a peak that is larger than LatestMaxIdx,
        but is within window_size window, then it will replace it.
        However, if LatestMaxIdx was replaced, it will be saved in PrevMaxIdx,
        and will be added if LatestMaxIdx was replaced again.
    */
    int LatestMaxIdx;
    int NextMaxIdx;
    double current_val;
    int flag;
    /* Current Coordinate */
    std::vector<std::tuple<int,int>> CoorVec;

    /* Process first window_size (or first window) elements of Spectrogram */
    int rdx;
    for (rdx = 0; rdx < window_size; ++rdx) {
        current_val = *(Spectrogram+rdx * num_cols+cdx);

        // For every element, the previous smaller elements are useless so remove them from Qi from rear
        while ((!Qi.empty()) && current_val >= *(Spectrogram+Qi.back() * num_cols+cdx)) { Qi.pop_back(); }

        // Add new element at rear of queue
        if (current_val > threshold_abs) { Qi.push_back(rdx); }
    }

    /* edge case :
        no max in the first window
        that satisfies the threshold
    */
    while (Qi.empty() && rdx < num_rows){
        current_val = *(Spectrogram+rdx * num_cols+cdx);
        if (current_val > threshold_abs) { Qi.push_back(rdx); }
        rdx++;
    }

    /* edge case :
        no local max at all in this column
    */
    if (rdx>=num_rows) {return;}

    LatestMaxIdx = Qi.front();
    CoorVec.push_back(std::make_tuple(LatestMaxIdx, cdx));
    // Process rest of the elements, i.e., from Spectrogram[rdx][window_size-1] to Spectrogram[rdx][num_cols-1]
    for (; rdx < num_rows; ++rdx) {
        current_val = *(Spectrogram+rdx * num_cols+cdx);

        // Remove the elements which are out of this window
        while ((!Qi.empty()) && Qi.front() < rdx - window_size)
        Qi.pop_front(); // Remove from front of queue

        // Remove all elements smaller than the currently
        // being added element (remove useless elements)
        while ((!Qi.empty()) && current_val >= *(Spectrogram+Qi.back() * num_cols+cdx))
            Qi.pop_back();

        // Add current element at the rear of Qi
        // only if it satisfies the threshold
        if (current_val > threshold_abs)
            Qi.push_back(rdx);
        else
            continue;

        // The element at the front of the queue is the largest element of
        // previous window, so store it to CoorVec
        if (Qi.front()!=LatestMaxIdx){
            NextMaxIdx = Qi.front();
            if ( NextMaxIdx-LatestMaxIdx>window_size ){
                // The peaks are window_size far apart.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(std::make_tuple(LatestMaxIdx, cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) < *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
                // If too near, and this is a greater val, replace the previous max.
                CoorVec.pop_back();
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(std::make_tuple(LatestMaxIdx,cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) == *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
                // If exactly the same, we retain it, even though it is too near.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(std::make_tuple(LatestMaxIdx,cdx));
            }
        }
    }

    /* Storing the filtered local max into peaks */
    // printf("\tCoorVec.size()=%lu",CoorVec.size());
    for (const auto &CurrCoor:CoorVec){
        flag=1;
        int adj_rdx = std::get<0>(CurrCoor);
        current_val = *(Spectrogram+adj_rdx * num_cols+cdx);

        // Filtering by adj rows
        for (int curr_cdx=cdx-window_size;     curr_cdx<cdx+window_size; ++curr_cdx){
            // Edge Cages
            if (curr_cdx==cdx     || curr_cdx<0 || curr_cdx>=num_cols) {continue;}
            // Proceed
            if (current_val < *(Spectrogram+adj_rdx * num_cols+curr_cdx)){
                flag=0;
                break;
            }
        }
        if (flag==0){continue;}

        // Filtering by 2D neighbourhood
        for (int curr_cdx=cdx-window_size;     curr_cdx<cdx+window_size; ++curr_cdx){
            // Edge Cages
            if (curr_cdx==cdx     || curr_cdx<0 || curr_cdx>=num_cols) {continue;}
            for (int curr_rdx=adj_rdx-window_size; curr_rdx<adj_rdx+window_size; ++curr_rdx){
                // Edge Cages
                if (curr_rdx==adj_rdx || curr_rdx<0 || curr_rdx>=num_rows) {continue;}

                // Proceed
                if (current_val < *(Spectrogram+curr_rdx * num_cols+curr_cdx)){
                    flag=0;
                    break;
                }
            }
        }

        // Add only if the adj rows dont have higher
        if (flag==1){
            // peaks.insert(CurrCoor);
            peaks.emplace_back(
                adj_rdx,
                cdx,
                current_val);
        }
    }
}

/* =============== get_raw_constellation_pts =============== */
void get_raw_constellation_pts(std::vector<std::tuple<int,int,double>> &peaks,
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    ){
    /* Get salient col coordinates */
    for (int cdx=base+1; cdx<nfft_half; cdx++){
        GetPeaks(
            (double*)Spectrogram, peaks,
            f_dim1, cdx, threshold_abs,
            nfft_half, numFrames);
    }
}

/* =============== get_adjPts =============== */
std::vector<std::tuple<int,int,double>> get_adjPts(
    int anchor0, int anchor1,
    std::vector<std::tuple<int,int,double>> &peaks,
    int delay_time, int delta_time, int delta_freq_half, int num_tgt_pts){
    // Init
    // delay_time  : Time delayed before seeking the next
    // delta_time  : Time width of Target Zone
    // delta_freq  : Freq width of Target Zone, will be +- half of this from the anchor point
    /* Target Zone Boundaries */
    // Time range
    int low_x = anchor0 + delay_time;
    int high_x = low_x + delta_time;
    // Freq range
    int low_y = anchor1 - delta_freq_half;
    int high_y = anchor1 + delta_freq_half;
    // Search Params
    int curr_peak0, curr_peak1, curr_peak2;
    std::vector<std::tuple<int,int,double>> candPts;

    // Search
    candPts.reserve(peaks.size());
    for(const auto& curr_peak : peaks){
        curr_peak0=std::get<0>(curr_peak);
        if (curr_peak0>low_x && curr_peak0<high_x){
            curr_peak1=std::get<1>(curr_peak);
            if (curr_peak1>low_y && curr_peak1<high_y){
                curr_peak2=std::get<2>(curr_peak);
                candPts.emplace_back(curr_peak0, curr_peak1, curr_peak2);
            }
        }
    }
    // Use heaps instead of sort
    make_heap(candPts.begin(), candPts.end(), Comp());

    // Select salient points
    std::vector<std::tuple<int,int,double>> adjPts;
    adjPts.reserve(candPts.size());
    for(int jdx=0; jdx<num_tgt_pts; ++jdx){
        if (candPts.size()==0){ break; }

        const auto &curr_candpt = candPts.front();
        adjPts.emplace_back(std::get<0>(curr_candpt), std::get<1>(curr_candpt), std::get<2>(curr_candpt));

        pop_heap(candPts.begin(), candPts.end(), Comp());
        candPts.pop_back();
    }
    return adjPts;
}

/* =============== spec2shash =============== */
std::unordered_map <int, std::set<int>> spec2shash_v4(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts,
    int delay_time, int delta_time, int delta_freq_half
    ){
    // Init
    std::unordered_map <int, std::set<int>> hash_DICT;
    int nfft_half = nfft/2;

    // Find peaks
    std::vector<std::tuple<int,int,double>> peaks;
    get_raw_constellation_pts(peaks,
        nfft_half, numFrames, (double*)x_MAG,
        f_dim1, threshold_abs, 20);
    printf("\t%s() : peaks = %lu \n", __func__, peaks.size() );

    /* Hashing : peaks -> adjPts -> hash_DICT */
    for(const auto& anchor_peak:peaks){
        // get anchor_peak.
        const auto &anchor0=std::get<0>(anchor_peak);
        const auto &anchor1=std::get<1>(anchor_peak);
        // adjPts : Find all points within the Target Zone of anchor_peak.
        // convert vector to heaps
        const auto &adjPts = get_adjPts(
            anchor0, anchor1, peaks,
            delay_time, delta_time, delta_freq_half, num_tgt_pts);
        // hash_DICT, curr_max
        for(const auto &curr_tuple:adjPts){
            // adjPts coords
            const auto &adj0=std::get<0>(curr_tuple);
            const auto &adj1=std::get<1>(curr_tuple);
            const auto &hash_key = (anchor1*1000000)+(adj1*1000)+(adj0-anchor0);
            // update hash_DICT
            if (hash_DICT.count(hash_key) > 0){
                hash_DICT[hash_key].insert(anchor0);
            }
            else{
                hash_DICT[hash_key] = { anchor0 };
            }
        }
    }
    return hash_DICT;
}

/* shashmatching sub-functions */
std::vector<int> get_Timeoffsets(
    std::unordered_map <int, std::set<int>> &hash_DICT1,
    std::unordered_map <int, std::set<int>> &hash_DICT2) {
    /* Init */
    std::vector<int> Time_offsets;
    Time_offsets.reserve(109);

    /* Get intersection */
    for (const auto& curr_pair2:hash_DICT2) {
        const auto& curr_key2 = curr_pair2.first;
        const auto& curr_set2 = curr_pair2.second;
        const auto& it1 = hash_DICT1.find(curr_key2);
        /* Find pairwise difference */
        if ( it1 != hash_DICT1.end() ){
            const auto& curr_set1 = it1->second;
            /* Calculate pairwise difference */
            for(auto& s1v : curr_set1) {
                for(auto& s2v : curr_set2) {
                    Time_offsets.push_back(s2v - s1v);
                }
            }
        }
    }
    return Time_offsets;
}
/* compute_histogram */
double* compute_histogram(double output_Hist[], std::vector<int> Time_offsets, int bins){
    /* Min Max to scale them from 0-100 */
    int curr_min = *min_element(Time_offsets.begin(), Time_offsets.end());
    int curr_max = *std::max_element(Time_offsets.begin(), Time_offsets.end())-curr_min;
    double curr_scle = 1.0*bins/(curr_max+1); // denom add 1 to avoid problem of 0

    /* Normalising it to [0,100] */
    int value;
    for (const auto &elem:Time_offsets){
        value = (elem - curr_min) * curr_scle;
        if (value == bins){
            output_Hist[bins-1] += 1;
        } else{
            output_Hist[value] += 1;
        }
    }

    return output_Hist;
}

/* shashmatching */
int shashmatching_v2(
    std::unordered_map <int, std::set<int>> &hash_DICT1,
    std::unordered_map <int, std::set<int>> &hash_DICT2,
    int &num_matches, int threshold_match, int bins){

    double hist[bins] = {0};
    std::vector<int> Time_offsets = get_Timeoffsets(hash_DICT1, hash_DICT2);
    int Time_offsets_size = Time_offsets.size();
    if ( Time_offsets_size < threshold_match ){
        printf("\t\t%s() : Time_offsets_size=%i < threshold_match=%i, supposed to skip... \n", __func__, Time_offsets_size, threshold_match);
        num_matches=0;
        // return 0;
    }

    compute_histogram(hist, Time_offsets, bins);
    num_matches = *std::max_element(hist , hist+bins);

    if ( num_matches > threshold_match ){
        return 1;
    } else {
        return 0;
    }
}

/*LUT sub-functions*/
// Passing a Look-Up-Table (LUT) in by reference, and adding
void addLUT(
    std::unordered_map <int, std::set<size_t>> &LUT,
    std::unordered_map <int, std::set<int>> &hash_DICT_db,
    size_t uttid){
    for (const auto &curr_keyval_pair:hash_DICT_db) {
        const auto &curr_key = curr_keyval_pair.first;
        if (LUT.count(curr_key) > 0){
            LUT[curr_key].insert( uttid );
        }
        else{
            LUT[curr_key] = { uttid };
        }
    }
}
// removing uttid and its hash from LUT
void eraseLUT(
    std::unordered_map <int, std::set<size_t> > &LUT,
    std::unordered_map <int, std::set<int>    > &hash_DICT_db,
    size_t uttid){

    for (const auto &curr_keyval_pair:hash_DICT_db) {
        const auto &curr_key = curr_keyval_pair.first;
        if (LUT.count(curr_key) > 0){
            LUT[curr_key].erase( uttid );
            if (LUT[curr_key].size() == 0){
                LUT.erase(curr_key);
            }
        }
    }

}
/* Searching the LUT to get Candidates
cand_set is sorted, by the ones with most common occurance */
std::vector<size_t> searchLUT_v2(
    std::unordered_map <int, std::set<size_t>> &LUT,
    std::unordered_map <int, std::set<int>> &query_hashmap,
    int TopN){

    // Create utt2occ_Counter
    std::unordered_map<size_t, int> utt2occ_Counter;
    for (const auto &hash2uttset:query_hashmap) {
        // Check if this hash is in the LUT
        auto curr_UttSet = LUT.find(hash2uttset.first);
        if(curr_UttSet == LUT.end()) { continue; }
        // Update Counter
        for (const auto it : curr_UttSet->second){
            // if (utt2occ_Counter.count(it) > 0)
            utt2occ_Counter[it]++;
        }
    }

    /* Update the occ2uttset_Counter & occ_Heap */
    std::unordered_map<int, std::set<size_t>> occ2uttset_Counter;
    std::vector<int> occ_Heap;
    for (const auto &utt2occ:utt2occ_Counter) {
        const auto &numocc = utt2occ.second;
        if (occ2uttset_Counter.count(numocc) > 0)
            occ2uttset_Counter[numocc].insert( utt2occ.first );
        else{
            occ2uttset_Counter[numocc] = { utt2occ.first };
            occ_Heap.push_back(numocc);
        }
    }
    make_heap(occ_Heap.begin(), occ_Heap.end());

    /* Update the candidates set, with early stopping */
    std::vector<size_t> CandSet;
    CandSet.reserve(TopN);
    int currOcc;
    int npt=0;
    while (occ_Heap.size()>0){
        /* Get the biggest element */
        currOcc = occ_Heap.front();
        /* Update CandSet */
        for (auto &it : occ2uttset_Counter[currOcc]){
            CandSet.push_back(it);
            /* Early Stopping */
            npt++;
            if (npt==TopN){
                return CandSet;
            }
        }
        /* Pop the biggest element */
        pop_heap(occ_Heap.begin(), occ_Heap.end());
        occ_Heap.pop_back();
    }

    return CandSet;
}
