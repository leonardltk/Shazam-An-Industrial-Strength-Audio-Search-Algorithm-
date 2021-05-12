/* -------------------------------------- imports -------------------------------------- */
#include "Shash.h"

// Defining typespace for unordered_map with tuple keys
namespace N {
    // Key type definition
    typedef tuple<int, int, int> key_t;
    // Hash definition for the key
    struct key_hash : public unary_function<key_t, size_t> {
        size_t operator()(const key_t& k) const {
            return get<0>(k) ^ get<1>(k) ^ get<2>(k);
        }
    };
    // KeyEqual definition for key
    struct key_equal : public binary_function<key_t, key_t, bool> {
        bool operator()(const key_t& v0, const key_t& v1) const {
            return (
                get<0>(v0) == get<0>(v1) &&
                get<1>(v0) == get<1>(v1) &&
                get<2>(v0) == get<2>(v1)
                );
        }
    };
    // shash
    typedef unordered_map<const key_t, unordered_set<int>, key_hash, key_equal> map_shash;
    // LUT
    typedef unordered_map<const key_t, unordered_set<string>, key_hash, key_equal> map_lut;
}
using namespace N;

/* =============== spec2shash sub-functions =============== */
/* Driver functions
    to sort the vector elements by third element of tuple
    https://www.geeksforgeeks.org/sorting-vector-tuple-c-ascending-order/
*/
// Ascending
bool sortbythird(const tuple<int,int,int>& a,
    const tuple<int,int,int>& b){
    return (get<2>(a) < get<2>(b));
}
// Descending
bool sortbythird_desc(const tuple<int,int,int>& a,
    const tuple<int,int,int>& b){
    return (get<2>(a) > get<2>(b));
}
// to use heaps to get the n largest peak magnitude value.
struct Comp {

    // // min heap
    // bool operator()(const tuple<int,int,int>& a,
    //         const tuple<int,int,int>& b){
    //        return (get<2>(a) > get<2>(b));
    // }

    // // max heap
    bool operator()(const tuple<int,int,int>& a,
        const tuple<int,int,int>& b){
        return (get<2>(a) < get<2>(b));
    }
};

/* =============== MAXLIST =============== */
/* MAXLIST function for rows and columns respectively */
void GetPeakCoor1D_row(double * Spectrogram, set<tuple<int,int>> &CoorSetRows,
    int window_size, int rdx, float threshold_abs,
    int num_cols){
    /* Implementing the MAXLIST algorithms to get local max */

    // Init
    /*
        Create a Double Ended Queue, Qi that will store indexes of Spectrogram elements
        The queue will store indexes of useful elements in current window and it will
        maintain decreasing order of values from front to rear in Qi,
        i.e., Spectrogram[rdx][Qi.front[]] to Spectrogram[rdx][Qi.rear()] are sorted in decreasing order
    */
    deque<int> Qi(window_size);
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
    /* Current Coordinate */
    vector<tuple<int,int>> CoorVec;

    /* Process first window_size (or first window) elements of Spectrogram */
    int cdx;
    for (cdx = 0; cdx < window_size; ++cdx) {
        current_val = *(Spectrogram+rdx * num_cols+cdx);

        /*For every element, the previous smaller elements are useless so
        remove them from Qi from rear */
        while ((!Qi.empty()) && current_val >= *(Spectrogram+rdx * num_cols+Qi.back())) { Qi.pop_back(); }

        // Add new element at rear of queue
        if (current_val > threshold_abs) { Qi.push_back(cdx); }
    }

    /* edge case :
        no max in the first window
        that satisfies the threshold
    */
    while (Qi.empty() && cdx < num_cols){
        current_val = *(Spectrogram+rdx * num_cols+cdx);
        if (current_val > threshold_abs)
            Qi.push_back(cdx);
        cdx++;
    }

    /* edge case :
        no local max in this column at all
    */
    if (cdx>=num_cols) {return;}

    LatestMaxIdx = Qi.front();
    CoorVec.push_back(make_tuple(rdx, LatestMaxIdx));
    // Process rest of the elements, i.e., from Spectrogram[rdx][window_size-1] to Spectrogram[rdx][num_cols-1]
    for (; cdx < num_cols; ++cdx) {
        double current_val = *(Spectrogram+rdx * num_cols+cdx);

        // Remove the elements which are out of this window
        while ((!Qi.empty()) && Qi.front() <= cdx - window_size)
        Qi.pop_front(); // Remove from front of queue

        // Remove all elements smaller than the currently
        // being added element (remove useless elements)
        while ((!Qi.empty()) && current_val >= *(Spectrogram+rdx * num_cols+Qi.back()))
            Qi.pop_back();

        // Add current element at the rear of Qi
        // only if it satisfies the threshold
        if (current_val > threshold_abs)
            Qi.push_back(cdx);
        else
            continue;

        // The element at the front of the queue is the largest element of
        // previous window, so store it to CoorVec
        if (Qi.front()!=LatestMaxIdx){
            NextMaxIdx = Qi.front();
            if ( NextMaxIdx-LatestMaxIdx>window_size ){
            // The peaks are window_size far apart.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(rdx, LatestMaxIdx));
            } else if (*(Spectrogram+rdx * num_cols+LatestMaxIdx) < *(Spectrogram+rdx * num_cols+NextMaxIdx)) {
            // If too near, and this is a greater val, replace the previous max.
                CoorVec.pop_back();
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(rdx, LatestMaxIdx));
            } else if (*(Spectrogram+rdx * num_cols+LatestMaxIdx) == *(Spectrogram+rdx * num_cols+NextMaxIdx)) {
            // If exactly the same, we retain it, even though it is too near.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(rdx, LatestMaxIdx));
            }
        }
    }
    /* Storing the local max into CoorSetRows */
    for (const auto &CurrCoor:CoorVec){
        CoorSetRows.insert(CurrCoor);
    }
}
void GetPeakCoor1D_col(double * Spectrogram, set<tuple<int,int>> &CoorSetCols,
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
    deque<int> Qi(window_size);
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
    /* Current Coordinate */
    vector<tuple<int,int>> CoorVec;

    /* Process first window_size (or first window) elements of Spectrogram */
    int rdx;
    for (rdx = 0; rdx < window_size; ++rdx) {
        current_val = *(Spectrogram+rdx * num_cols+cdx);

    /*For every element, the previous smaller elements are useless so
    remove them from Qi from rear */
    while ((!Qi.empty()) && current_val >= *(Spectrogram+Qi.back() * num_cols+cdx)) { Qi.pop_back(); }

    // Add new element at rear of queue
    if (current_val > threshold_abs) Qi.push_back(rdx); }

    /* edge case :
        no max in the first window
        that satisfies the threshold
    */
    while (Qi.empty() && rdx < num_rows){
        current_val = *(Spectrogram+rdx * num_cols+cdx);
        if (current_val > threshold_abs)
            Qi.push_back(rdx);
        rdx++;
    }

    /* edge case :
        no local max at all in this row
    */
    if (rdx>=num_rows) {return;}

    LatestMaxIdx = Qi.front();
    CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
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
                CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) < *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
            // If too near, and this is a greater val, replace the previous max.
                CoorVec.pop_back();
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) == *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
            // If exactly the same, we retain it, even though it is too near.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            }
        }
    }

    /* Storing the local max into CoorSetCols */
    for (const auto &CurrCoor:CoorVec){
        CoorSetCols.insert(CurrCoor);
    }
}
/* Speed up : still perform MAXLIST on columns, but each peak is further filtered by looking at its row. */
void GetPeakCoor1D_col_filterrow(double * Spectrogram,
    vector<tuple<int,int,double>> &CoorSetCols,
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
    deque<int> Qi(window_size);
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
    vector<tuple<int,int>> CoorVec;

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
    CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
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
                CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) < *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
            // If too near, and this is a greater val, replace the previous max.
                CoorVec.pop_back();
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) == *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
            // If exactly the same, we retain it, even though it is too near.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            }
        }
    }

    /* Storing the local max into CoorSetCols */
    for (const auto &CurrCoor:CoorVec){
        flag=1;
        int adj_rdx = get<0>(CurrCoor);
        current_val = *(Spectrogram+adj_rdx * num_cols+cdx);
        // Filtering by adj rows
        for (int curr_cdx=cdx-window_size; curr_cdx<cdx+window_size; ++curr_cdx){
            // Edge Cages
            if (curr_cdx==cdx || curr_cdx<0 || curr_cdx>=num_cols) {continue;}
            // Proceed
            if (current_val < *(Spectrogram+adj_rdx * num_cols+curr_cdx)){
                flag=0;
                break;
            }
        }
        // Add only if the adj rows dont have higher
        if (flag==1){
            // CoorSetCols.insert(CurrCoor);
            CoorSetCols.emplace_back(
                adj_rdx,
                cdx,
                current_val);
        }
    }
}
/* Speed up : GetPeakCoor1D_col_filterrow, but each peak is further filtered by looking at its 2d neighbourhood. */
void GetPeakCoor1D_col_filter2D(double * Spectrogram,
    vector<tuple<int,int,double>> &CoorSetCols,
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
    deque<int> Qi(window_size);
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
    vector<tuple<int,int>> CoorVec;

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
    CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
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
                CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) < *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
                // If too near, and this is a greater val, replace the previous max.
                CoorVec.pop_back();
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) == *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
                // If exactly the same, we retain it, even though it is too near.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            }
        }
    }

    /* Storing the filtered local max into CoorSetCols */
    for (const auto &CurrCoor:CoorVec){
        flag=1;
        int adj_rdx = get<0>(CurrCoor);
        current_val = *(Spectrogram+adj_rdx * num_cols+cdx);

        // Filtering by adj rows
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
            // CoorSetCols.insert(CurrCoor);
            CoorSetCols.emplace_back(
                adj_rdx,
                cdx,
                current_val);
        }
    }
}
/* Speed up : stillGetPeakCoor1D_col_filter2D but
check row, before check 2d neighbours
To improve : check confirmed candidates coordinates to reduce 2d search time
*/
void GetPeakCoor1D_col_filterrow2D(double * Spectrogram,
    vector<tuple<int,int,double>> &peaks,
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
    deque<int> Qi(window_size);
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
    vector<tuple<int,int>> CoorVec;

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
    CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
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
                CoorVec.push_back(make_tuple(LatestMaxIdx, cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) < *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
                // If too near, and this is a greater val, replace the previous max.
                CoorVec.pop_back();
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            } else if (*(Spectrogram+LatestMaxIdx * num_cols+cdx) == *(Spectrogram+NextMaxIdx * num_cols+cdx)) {
                // If exactly the same, we retain it, even though it is too near.
                LatestMaxIdx=NextMaxIdx;
                CoorVec.push_back(make_tuple(LatestMaxIdx,cdx));
            }
        }
    }

    /* Storing the filtered local max into peaks */
    // printf("\tCoorVec.size()=%lu",CoorVec.size());
    for (const auto &CurrCoor:CoorVec){
        flag=1;
        int adj_rdx = get<0>(CurrCoor);
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
/* Naive O(nm*kk) : Raw extraction of the local max, by 2D windowing */
vector<tuple<int,int,double>> get_raw_constellation_pts_v1(
    int nfft_half, int numFrames, double * x_MAG,
    int t_dim1, int f_dim1,
    int t_dim2, int f_dim2,
    float threshold_abs, int base){

    vector<tuple<int,int,double>> peaks;
    int xdx, ydx;
    double currpeak[3] = {0,0,0};
    double curr_val;
    int flag;

    // finding local max
    // iterate xdx from 0 to numFrames, hop of t_dim1
    // iterate ydx from 0 to nfft_half, hop of f_dim1
    xdx = 0;
    while (xdx+t_dim1 < numFrames) {
        ydx = 0;
        while (ydx+f_dim1 < nfft_half) {
            currpeak[2] = threshold_abs;
            flag=0;
            for (int ii=0; ii < t_dim2; ii++){
                for (int jj=0; jj < f_dim2; jj++){
                    if (ydx+jj <= base){ continue; }
                    curr_val = *(x_MAG+(xdx+ii) * nfft_half+(ydx+jj));
                    if (curr_val > currpeak[2]) {
                        currpeak[0] = xdx+ii;
                        currpeak[1] = ydx+jj;
                        currpeak[2] = curr_val;
                        flag=1;
                    }
                }
            }
            if (flag==1){
                peaks.emplace_back(currpeak[0], currpeak[1], currpeak[2]);
            }
            ydx += f_dim1;
        }
        xdx += t_dim1;
    }
    return peaks;
}
/* O(nm) :
Approx conversion of python's : coordinates1 = skimage.feature.peak_local_max(SPECTROGRAM, min_distance=, threshold_abs=, indices=True)
MAXLIST algo along rows and by columns, then find their intersection.
*/
vector<tuple<int,int,double>> get_raw_constellation_pts_v2(
    int nfft_half, int numFrames, double * Spectrogram,
    int t_dim1, int f_dim1, float threshold_abs, int base
    ){
    // Init
    set<tuple<int,int>> CoorSetRows;
    set<tuple<int,int>> CoorSetCols;
    vector<tuple<int,int>> CoorIntersect;
    vector<tuple<int,int,double>> peaks;

    // Get salient row coordinates
    for (int rdx=0; rdx<numFrames; rdx++){
        GetPeakCoor1D_row((double*)Spectrogram, CoorSetRows,
            t_dim1, rdx, threshold_abs,
            nfft_half);
    }
    // Get salient col coordinates
    for (int cdx=0; cdx<nfft_half; cdx++){
        GetPeakCoor1D_col((double*)Spectrogram, CoorSetCols,
            f_dim1, cdx, threshold_abs,
            nfft_half, numFrames);
    }
    // CoorIntersect <- Get intersection of CoorSetRows & CoorSetCols
    set_intersection(
        CoorSetRows.begin(),CoorSetRows.end(),
        CoorSetCols.begin(),CoorSetCols.end(),
        inserter(CoorIntersect,CoorIntersect.begin()));
    // peaks <- CoorIntersect
    peaks.reserve(CoorIntersect.size());
    for(const auto &curr_coor:CoorIntersect){
    // Get the coordinates
        const auto &col_idx = get<1>(curr_coor);
        if (col_idx <= base){ continue; }
        const auto &row_idx = get<0>(curr_coor);
    // update the peaks with the coordinates and the spectrogram value
        peaks.emplace_back(row_idx, col_idx, *(Spectrogram+row_idx * (nfft_half)+col_idx));
    }
    return peaks;
}
/* Edit on _v2() : Post-process results of GetPeakCoor1D_col_filterrow, instead of running both row & col
    just 1 pass of each col
    then for each selected peak,
    just check whether it is indeed the local max in its column window.
*/
vector<tuple<int,int,double>> get_raw_constellation_pts_v3(
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    ){
    // Init
    vector<tuple<int,int,double>> peaks;

    // Get salient col coordinates
    for (int cdx=base+1; cdx<nfft_half; cdx++){
        GetPeakCoor1D_col_filterrow(
            (double*)Spectrogram, peaks,
            f_dim1, cdx, threshold_abs,
            nfft_half, numFrames);
    }
    return peaks;
}
/* Edit on _v3() : Use GetPeakCoor1D_col_filter2D instead of GetPeakCoor1D_col_filterrow
*/
vector<tuple<int,int,double>> get_raw_constellation_pts_v4(
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    ){
    // Init
    vector<tuple<int,int,double>> peaks;

    // Get salient col coordinates
    for (int cdx=base+1; cdx<nfft_half; cdx++){
        GetPeakCoor1D_col_filter2D(
            (double*)Spectrogram, peaks,
            f_dim1, cdx, threshold_abs,
            nfft_half, numFrames);
    }

    return peaks;
}
/* Edit on _v4() :
    Use GetPeakCoor1D_col_filterrow2D instead of GetPeakCoor1D_col_filterrow
    Pass peak in by reference, instead of creating and copying it out.
*/
void get_raw_constellation_pts_v5(vector<tuple<int,int,double>> &peaks,
    int nfft_half, int numFrames, double * Spectrogram,
    int f_dim1, float threshold_abs, int base
    ){
    // Get salient col coordinates
    for (int cdx=base+1; cdx<nfft_half; cdx++){
        GetPeakCoor1D_col_filterrow2D(
            (double*)Spectrogram, peaks,
            f_dim1, cdx, threshold_abs,
            nfft_half, numFrames);
    }
}

/* =============== get_adjPts =============== */
// This gets the adj points in order, ignoring the magnitude value, then use sort
vector<tuple<int,int,double>> get_adjPts_v1(
    int anchor0, int anchor1,
    vector<tuple<int,int,double>> &peaks,
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
    tuple<int,int,double> curr_peak;
    int curr_peak0, curr_peak1, curr_peak2;
    vector<tuple<int,int,double>> candPts;

    // Search
    for(unsigned jdx =0; jdx<peaks.size(); jdx++){
        curr_peak=peaks[jdx];
        curr_peak0=get<0>(curr_peak);
        if (curr_peak0>low_x && curr_peak0<high_x){
            curr_peak1=get<1>(curr_peak);
            if (curr_peak1>low_y && curr_peak1<high_y){
                curr_peak2=get<2>(curr_peak);
                candPts.emplace_back(curr_peak0, curr_peak1, curr_peak2);
            }
        }
    }
    sort(candPts.begin(), candPts.end(), sortbythird_desc);

    // Select salient points
    vector<tuple<int,int,double>> adjPts;
    int candPts_size = candPts.size();
    if (candPts_size < num_tgt_pts){
        for(int jdx =0; jdx<candPts_size; jdx++){
            adjPts.emplace_back(get<0>(candPts[jdx]), get<1>(candPts[jdx]), get<2>(candPts[jdx]));
        }
    }
    else{
        for(int jdx =0; jdx<num_tgt_pts; jdx++){
            adjPts.emplace_back(get<0>(candPts[jdx]), get<1>(candPts[jdx]), get<2>(candPts[jdx]));
        }
    }
    return adjPts;
}
// Replacing sort with heaps
vector<tuple<int,int,double>> get_adjPts_v2(
    int anchor0, int anchor1,
    vector<tuple<int,int,double>> &peaks,
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
    vector<tuple<int,int,double>> candPts;

    // Search
    candPts.reserve(peaks.size());
    for(const auto& curr_peak : peaks){
        curr_peak0=get<0>(curr_peak);
        if (curr_peak0>low_x && curr_peak0<high_x){
            curr_peak1=get<1>(curr_peak);
            if (curr_peak1>low_y && curr_peak1<high_y){
                curr_peak2=get<2>(curr_peak);
                candPts.emplace_back(curr_peak0, curr_peak1, curr_peak2);
            }
        }
    }
    // Use heaps instead of sort
    make_heap(candPts.begin(), candPts.end(), Comp());

    // Select salient points
    vector<tuple<int,int,double>> adjPts;
    adjPts.reserve(candPts.size());
    for(int jdx=0; jdx<num_tgt_pts; ++jdx){
        if (candPts.size()==0){ break; }

        const auto &curr_candpt = candPts.front();
        adjPts.emplace_back(get<0>(curr_candpt), get<1>(curr_candpt), get<2>(curr_candpt));

        pop_heap(candPts.begin(), candPts.end(), Comp());
        candPts.pop_back();
    }
    return adjPts;
}
// Construct heaps from scratch : but it is slower than v2
vector<tuple<int,int,double>> get_adjPts_v3(
    int anchor0,
    int anchor1,
    vector<tuple<int,int,double>> &peaks,
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

    // Search
    vector<tuple<int,int,double>> candPts;
    for(unsigned jdx =0; jdx<peaks.size(); jdx++){
        tuple<int,int,double> curr_peak=peaks[jdx];
        int curr_peak0=get<0>(curr_peak);
        if (curr_peak0>low_x){
            if (curr_peak0<high_x){
                int curr_peak1=get<1>(curr_peak);
                if (curr_peak1>low_y){
                    if (curr_peak1<high_y){
                        int curr_peak2=get<2>(curr_peak);

                        candPts.emplace_back(curr_peak0, curr_peak1, curr_peak2);
                        push_heap(candPts.begin(), candPts.end(), Comp());

                    }
                }
            }
        }
    }

    // Select salient points
    vector<tuple<int,int,double>> adjPts;
    adjPts.reserve(candPts.size());
    tuple<int,int,double> tmpa;
    int candPts_size = candPts.size();
    if (candPts_size < num_tgt_pts){
        for(int jdx =0; jdx<candPts_size; jdx++){
            tmpa = candPts.front();
            pop_heap(candPts.begin(), candPts.end(), Comp());
            candPts.pop_back();
            adjPts.emplace_back(get<0>(tmpa), get<1>(tmpa), get<2>(tmpa));
        }
    }
    else{
        for(int jdx =0; jdx<num_tgt_pts; jdx++){
            tmpa = candPts.front();
            pop_heap(candPts.begin(), candPts.end(), Comp());
            candPts.pop_back();
            adjPts.emplace_back(get<0>(tmpa), get<1>(tmpa), get<2>(tmpa));
        }
    }
    return adjPts;
}

/* =============== Read/Write =============== */
// Read from file
unordered_map<int, set<int>> txt2hash(const string &fileName) {
    unordered_map<int, set<int>> targetMap;

    ifstream myStream(fileName);
    if (!myStream.is_open()) {
        printf("%s() : Failed to open %s \n", __func__, fileName.c_str());
        throw;
        return targetMap;
    }

    int hashkey, set_size, val;
    set<int> SetOfTimeStamps;
    while (myStream >> hashkey >> set_size) {
        for (int i = 0; i < set_size; i++) {
            myStream >> val;
            SetOfTimeStamps.insert(val);
        };
        targetMap[hashkey] = SetOfTimeStamps;
        SetOfTimeStamps.clear();
    }

    return targetMap;
}
void get_db2bins(
    const string &db_db2bin,
    unordered_map <string, int> &uttDB2bins_DICT) {
    printf("\n\tReading from db_db2bin=%s\n", db_db2bin.c_str());

    string uttid;
    int numBins;
    ifstream f_db_db2bin(db_db2bin.c_str());
    if (f_db_db2bin.is_open()) {
        while (f_db_db2bin >> uttid >> numBins) {
            uttDB2bins_DICT[uttid] = numBins;
        }
        f_db_db2bin.close();
    }

    printf("\tFinished reading from db_db2bin=%s\n", db_db2bin.c_str());
    printf("\tuttDB2bins_DICT.size() = %lu \n", uttDB2bins_DICT.size());
}

// Write to file
void peaks2txt(const string &fileName, vector<tuple<int,int,double>> &peaks) {
    ofstream myFile(fileName);
    if (!myFile.is_open()) {
        // printf("%s() : Failed to open %s \n", __func__, fileName.c_str());
        cout << __func__ << "() : Failed to open " << fileName << endl;
        throw;
        return;
    }

    for (const auto &it : peaks) {
        myFile << get<0>(it) << " " << get<1>(it) << " " << get<2>(it) << endl;
    }
    myFile.flush();
}
void hash2txt(const string &fileName, unordered_map<int, set<int>> &map_in) {
    ofstream myFile(fileName);
    if (!myFile.is_open()) {
        // printf("%s() : Failed to open %s \n", __func__, fileName.c_str());
        cout << __func__ << "() : Failed to open " << fileName << endl;
        throw;
        return;
    }

    for (const auto &it:map_in) {
        // int_key
        myFile << it.first << " ";
        // tuple_key
        // myFile << get<0>(it.first) << " " << get<1>(it.first) << " " << get<2>(it.first) << " ";

        // set_values
        myFile << it.second.size() << " ";
        for (const auto &it1 : it.second) {
            myFile << it1 << " ";
        }
        myFile << endl;
    }
    myFile.flush();
}

/* =============== spec2shash =============== */
/* Using get_raw_constellation_pts_v2 */
unordered_map <int, set<int>> spec2shash_v1(
    double * x_MAG, int numFrames, int nfft,
    int t_dim1, int f_dim1, float threshold_abs, int num_tgt_pts,
    int delay_time, int delta_time, int delta_freq_half
    ){
    // Init
    unordered_map <int, set<int>> hash_DICT;
    int nfft_half = nfft/2;

    // Find peaks
    vector<tuple<int,int,double>> peaks = get_raw_constellation_pts_v2(
        nfft_half, numFrames, (double*)x_MAG,
        t_dim1, f_dim1, threshold_abs, 20);
    // printf("%s() : Number of peaks=%lu\n", __func__, peaks.size());

    // Hashing : peaks -> adjPts -> hash_DICT
    // printf("\n%s() : --Hashing : peaks -> adjPts -> hash_DICT\n", __func__);
    for(const auto& anchor_peak:peaks){
        // get anchor_peak.
        const auto &anchor0=get<0>(anchor_peak);
        const auto &anchor1=get<1>(anchor_peak);
        // adjPts : Find all points within the Target Zone of anchor_peak.
        // convert vector to heaps
        const auto &adjPts = get_adjPts_v2(
            anchor0, anchor1, peaks,
            delay_time, delta_time, delta_freq_half, num_tgt_pts);
        // hash_DICT, curr_max
        for(const auto &curr_tuple:adjPts){  // for(unsigned jdx =0; jdx < adjPts.size(); jdx++){
            // adjPts coords
            const auto &adj0=get<0>(curr_tuple);  //     adj0=get<0>(adjPts[jdx]);
            const auto &adj1=get<1>(curr_tuple);  //     adj1=get<1>(adjPts[jdx]);
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
    // printf("%s() : Number of hash_DICT=%lu\n", __func__, hash_DICT.size());
    return hash_DICT;
}
/* Using get_raw_constellation_pts_v3 */
unordered_map <int, set<int>> spec2shash_v2(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts,
    int delay_time, int delta_time, int delta_freq_half
    ){
    // Init
    unordered_map <int, set<int>> hash_DICT;
    int nfft_half = nfft/2;

    // Find peaks
    vector<tuple<int,int,double>> peaks = get_raw_constellation_pts_v3(
        nfft_half, numFrames, (double*)x_MAG,
        f_dim1, threshold_abs, 20);

    // Hashing : peaks -> adjPts -> hash_DICT
    for(const auto& anchor_peak:peaks){
        // get anchor_peak.
        const auto &anchor0=get<0>(anchor_peak);
        const auto &anchor1=get<1>(anchor_peak);
        // adjPts : Find all points within the Target Zone of anchor_peak.
        // convert vector to heaps
        const auto &adjPts = get_adjPts_v2(
            anchor0, anchor1, peaks,
            delay_time, delta_time, delta_freq_half, num_tgt_pts);
        // hash_DICT, curr_max
        for(const auto &curr_tuple:adjPts){  // for(unsigned jdx =0; jdx < adjPts.size(); jdx++){
            // adjPts coords
            const auto &adj0=get<0>(curr_tuple);  //     adj0=get<0>(adjPts[jdx]);
            const auto &adj1=get<1>(curr_tuple);  //     adj1=get<1>(adjPts[jdx]);
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
/* Using get_raw_constellation_pts_v4 */
unordered_map <int, set<int>> spec2shash_v3(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts,
    int delay_time, int delta_time, int delta_freq_half
    ){
    // Init
    unordered_map <int, set<int>> hash_DICT;
    int nfft_half = nfft/2;

    // Find peaks
    vector<tuple<int,int,double>> peaks = get_raw_constellation_pts_v4(
        nfft_half, numFrames, (double*)x_MAG,
        f_dim1, threshold_abs, 20);

    // Hashing : peaks -> adjPts -> hash_DICT
    for(const auto& anchor_peak:peaks){
        // get anchor_peak.
        const auto &anchor0=get<0>(anchor_peak);
        const auto &anchor1=get<1>(anchor_peak);
        // adjPts : Find all points within the Target Zone of anchor_peak.
        // convert vector to heaps
        const auto &adjPts = get_adjPts_v2(
            anchor0, anchor1, peaks,
            delay_time, delta_time, delta_freq_half, num_tgt_pts);
        // hash_DICT, curr_max
        for(const auto &curr_tuple:adjPts){  // for(unsigned jdx =0; jdx < adjPts.size(); jdx++){
            // adjPts coords
            const auto &adj0=get<0>(curr_tuple);  //     adj0=get<0>(adjPts[jdx]);
            const auto &adj1=get<1>(curr_tuple);  //     adj1=get<1>(adjPts[jdx]);
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
/* Using get_raw_constellation_pts_v5 */
unordered_map <int, set<int>> spec2shash_v4(
    double * x_MAG, int numFrames, int nfft,
    int f_dim1, float threshold_abs, int num_tgt_pts,
    int delay_time, int delta_time, int delta_freq_half
    ){
    // Init
    unordered_map <int, set<int>> hash_DICT;
    int nfft_half = nfft/2;

    // Find peaks
    vector<tuple<int,int,double>> peaks;
    get_raw_constellation_pts_v5(peaks,
        nfft_half, numFrames, (double*)x_MAG,
        f_dim1, threshold_abs, 20);

    // Hashing : peaks -> adjPts -> hash_DICT
    for(const auto& anchor_peak:peaks){
        // get anchor_peak.
        const auto &anchor0=get<0>(anchor_peak);
        const auto &anchor1=get<1>(anchor_peak);
        // adjPts : Find all points within the Target Zone of anchor_peak.
        // convert vector to heaps
        const auto &adjPts = get_adjPts_v2(
            anchor0, anchor1, peaks,
            delay_time, delta_time, delta_freq_half, num_tgt_pts);
        // hash_DICT, curr_max
        for(const auto &curr_tuple:adjPts){  // for(unsigned jdx =0; jdx < adjPts.size(); jdx++){
            // adjPts coords
            const auto &adj0=get<0>(curr_tuple);  //     adj0=get<0>(adjPts[jdx]);
            const auto &adj1=get<1>(curr_tuple);  //     adj1=get<1>(adjPts[jdx]);
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
//
/*shashmatching sub-functions*/
/* get_Timeoffsets */
vector<int> get_Timeoffsets_v1(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2) {
    // Init
    vector<int> Time_offsets;
    Time_offsets.reserve(55);

    // Get intersection
    for (const auto& curr_pair2:hash_DICT2) {
        const auto& curr_key2 = curr_pair2.first;
        const auto& curr_set2 = curr_pair2.second;
        const auto& it1 = hash_DICT1.find(curr_key2);
        // Find pairwise difference
        if ( it1 != hash_DICT1.end() ){
            const auto& curr_set1 = it1->second;
            // Calculate pairwise difference
            for(auto& s1v : curr_set1) {
                for(auto& s2v : curr_set2) {
                    Time_offsets.push_back(s2v - s1v);
                }
            }
        }
    }
    return Time_offsets;
}
// make_decision (unused for now)
int make_decision_v1(vector<int> Time_offsets) {
    map <int,int> Counter_offset;
    // cout << "\n == Counter_offset :\n    ";
    double curr_min = *min_element(Time_offsets.begin(), Time_offsets.end());
    double curr_max = *max_element(Time_offsets.begin(), Time_offsets.end())-curr_min;
    double curr_scle = 0.5*(100-1)/(curr_max-1);
    for (unsigned i = 0; i < Time_offsets.size(); i++) {
        // Normalising it to [0,100]
        int curr_val = (Time_offsets[i] - curr_min) * curr_scle;
        // update Counter_offset
        if (Counter_offset.count(curr_val) > 0){
            Counter_offset[curr_val] += 1;
        } else{
            Counter_offset[curr_val] = 1;
        }
    }

    // Printing
    cout << "\n -- print Counter_offset :\n    ";
    // Get keys
    vector<int> _KEYS;
    _KEYS.reserve(Counter_offset.size());
    for(auto const& imap: Counter_offset){
        _KEYS.push_back(imap.first);
    }

    // Print for each key
    for(unsigned jdx =0; jdx < _KEYS.size(); jdx++){
        cout << "\n    Counter_offset[" << _KEYS[jdx] << "]\t="
        << Counter_offset[_KEYS[jdx]];
    }

    // Get keys
    vector<int> KEYS;
    KEYS.reserve(Counter_offset.size());
    for(auto const& imap: Counter_offset){
        KEYS.push_back(imap.first);
    }

    // Print for each key
    for(unsigned jdx =0; jdx < KEYS.size(); jdx++){
        if (Counter_offset[KEYS[jdx]] > 10){
            cout << "\nThese shash match : num matches = " << Counter_offset[KEYS[jdx]];
            return 1;
        }
    }
    cout << "\nThese shash dont match.";
    return 0;
}
/* Augmented Histogram */
int make_decision_v2(vector<int> Time_offsets) {
    map <int,int> Counter_offset;
    double curr_min = *min_element(Time_offsets.begin(), Time_offsets.end());
    double curr_max = *max_element(Time_offsets.begin(), Time_offsets.end())-curr_min;
    double curr_scle = 0.5*(100-1)/(curr_max-1);
    for (unsigned i = 0; i < Time_offsets.size(); i++) {
        // Normalising it to [0,100]
        int curr_val = (Time_offsets[i] - curr_min) * curr_scle;
        // update Counter_offset
        if (Counter_offset.count(curr_val) > 0){
            Counter_offset[curr_val] += 1;
            if (Counter_offset[curr_val] > 10){
                cout << "\nThese shash match : num matches = " << Counter_offset[curr_val];
                return 1;
            }
        } else{
            Counter_offset[curr_val] = 1;
        }
    }
    cout << "\nThese shash dont match.";
    return 0;
}

// compute_histogram
double* compute_histogram(double output_Hist[], vector<int> Time_offsets, int bins){
    // Min Max to scale them from 0-100
    int curr_min = *min_element(Time_offsets.begin(), Time_offsets.end());
    int curr_max = *max_element(Time_offsets.begin(), Time_offsets.end())-curr_min;
    double curr_scle = 1.0*bins/(curr_max+1); // denom add 1 to avoid problem of 0

    // Normalising it to [0,100]
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
// Using make_decision_v2
int shashmatching_v0(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2){
    // get Time_offsets
    vector<int> Time_offsets = get_Timeoffsets_v1(hash_DICT1, hash_DICT2);
    // make shash_decision
    // int shash_decision = make_decision_v1(Time_offsets);
    int shash_decision = make_decision_v2(Time_offsets);
    return shash_decision;
}
int shashmatching_v1(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2,
    int &num_matches, int threshold_match, int bins){
    // Init
    double hist[bins] = {0};

    vector<int> Time_offsets = get_Timeoffsets_v1(hash_DICT1, hash_DICT2);
    compute_histogram(hist, Time_offsets, bins);
    num_matches = *max_element(hist , hist+bins);

    if ( num_matches > threshold_match){
        return 1;
    } else {
        return 0;
    }
}
/* v1 but :
    Ignore if Time_offsets.size() or hash_DICT* < threshold_match
    Using histogram with variable bins
*/
int shashmatching_v2(
    unordered_map <int, set<int>> &hash_DICT1,
    unordered_map <int, set<int>> &hash_DICT2,
    int &num_matches, int threshold_match, int bins){
    // Init
    double hist[bins] = {0};
    // int hash_DICT1_size = hash_DICT1.size();
    // int hash_DICT2_size = hash_DICT2.size();
    // if ( hash_DICT1_size<threshold_match || hash_DICT2_size<threshold_match){
    //     printf("\t\t%s() : hash_DICT1_size||hash_DICT2_size < (%i=threshold_match). \n", __func__, threshold_match);
    //     num_matches=0;
    //     return 0;
    // }

    vector<int> Time_offsets = get_Timeoffsets_v1(hash_DICT1, hash_DICT2);
    // const auto &Time_offsets = get_Timeoffsets_v1(hash_DICT1, hash_DICT2);
    int Time_offsets_size = Time_offsets.size();
    if ( Time_offsets_size<threshold_match){
        // printf("\t\t%s() : (Time_offsets_size=%i) < (%i=threshold_match). \n", __func__, Time_offsets_size, threshold_match);
        num_matches=0;
        return 0;
    }

    compute_histogram(hist, Time_offsets, bins);
    num_matches = *max_element(hist , hist+bins);

    if ( num_matches > threshold_match){
        // printf("\t\t%s() : These shash match : (num_matches=%i) > (%i=threshold_match)\n", __func__, num_matches, threshold_match);
        return 1;
    } else {
        // printf("\t\t%s() : These shash dont match : (num_matches=%i) <= (%i=threshold_match)\n", __func__, num_matches, threshold_match);
        return 0;
    }
}

/*LUT sub-functions*/
// Passing a Look-Up-Table (LUT) in by reference, and adding
void addLUT(
    unordered_map <int, set<string>> &LUT,
    unordered_map <int, set<int>> &hash_DICT_db,
    const string uttid){
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
// Searching the LUT to get Candidates
/* cand_set is randomly arranged */
set<string> searchLUT_v1(
    unordered_map <int, set<string>> &LUT,
    unordered_map <int, set<int>> &query_hashmap){

    set<string> cand_set;

    for (const auto &curr_keyval_pair:query_hashmap) {
        const auto &curr_key = curr_keyval_pair.first;
        // Check if this hash is in the LUT
        if (LUT.count(curr_key) > 0){
            // Update the candidates set
            const auto &curr_set = LUT[curr_key];
            // for (auto it=curr_set.begin(); it != curr_set.end(); ++it)
                // cand_set.insert(*it);
            for (const auto &it : curr_set)
                cand_set.insert(it);
        }
    }
    return cand_set;
}
/* cand_set is sorted, by the ones with most common occurance */
vector<string> searchLUT_v2(
    unordered_map <int, set<string>> &LUT,
    unordered_map <int, set<int>> &query_hashmap,
    int TopN){

    // Create utt2occ_Counter
    unordered_map<string, int> utt2occ_Counter;
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

    // Update the occ2uttset_Counter & occ_Heap
    unordered_map<int, set<string>> occ2uttset_Counter;
    vector<int> occ_Heap;
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

    // Update the candidates set, with early stopping
    vector<string> CandSet;
    CandSet.reserve(TopN);
    int currOcc;
    int npt=0;
    while (occ_Heap.size()>0){
        // Get the biggest element
        currOcc = occ_Heap.front();
        // Update CandSet
        for (auto &it : occ2uttset_Counter[currOcc]){
            // CandSet[npt]=it;
            CandSet.push_back(it);
            // Early Stopping
            npt++;
            if (npt==TopN){
                return CandSet;
            }
        }
        // Pop the biggest element
        pop_heap(occ_Heap.begin(), occ_Heap.end());
        occ_Heap.pop_back();
    }

    return CandSet;
}

// Read from file
unordered_map<int, set<string>> txt2LUT(const string &fileName) {
    unordered_map<int, set<string>> targetMap;

    ifstream myStream(fileName);
    if (!myStream.is_open()) {
        printf("%s() : Failed to open %s \n", __func__, fileName.c_str());
        throw;
        return targetMap;
    }

    int hashkey, set_size;
    set<string> SetOfTimeStamps;
    while (myStream >> hashkey >> set_size) {
        for (int i = 0; i < set_size; i++) {
            string val;
            myStream >> val;
            SetOfTimeStamps.insert(val);
        }
        targetMap[hashkey] = SetOfTimeStamps;
        SetOfTimeStamps.clear();
    }

    printf("%s() : Read from %s \n", __func__, fileName.c_str());
    return targetMap;
}
// Write to file
void LUT2txt(const string &fileName, unordered_map<int, set<string>> &map_in) {
    ofstream myFile(fileName);
    if (!myFile.is_open()) {
        // printf("%s() : Failed to open %s \n", __func__, fileName.c_str());
        cout << __func__ << "() : Failed to open " << fileName << endl;
        throw;
        return;
    }

    for (const auto &it:map_in) {
        // int_key
        myFile << it.first << " ";

        // set_values
        myFile << it.second.size()
        << " "; // i'll be easier to set values read from file.. otherwise we need to use stringstream
        for (const auto &it1: it.second) {
            myFile << it1 << " ";
        }
        myFile << endl;
    }
    myFile.flush();

    printf("%s() : Written to %s \n", __func__, fileName.c_str());
}
