#ifndef SHASH_FASTAUDIOSHASHMANAGER_H
#define SHASH_FASTAUDIOSHASHMANAGER_H

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <bitset>
#include <tbb/tbb.h>
#include <tbb/task_scheduler_init.h>
#include <chrono>

std::string encode_hash(const std::unordered_map <int, std::set<int>>& hash);

std::unordered_map <int, std::set<int>> decode_hash(std::string hash_str);

class FastAudioShashManager {
private:
    /*
    Vid :
        video id, also the cluster id to represent a group,
        the unique identifier for the input audio segment,
        that consists of multiple Sid that are broken up from the original
    Sid :
        segment id
        in increasing order, the number used to connect to Vid as well as their individual hashes
    */

    /*  Vid <-> Sid */
    std::unordered_map <size_t, uint64_t> Sid2Vid;
    /* Sid2Vid = {
        1001 : 1,
        1002 : 1,
        1003 : 1,
        }
    */
    std::unordered_map <uint64_t, std::unordered_map <size_t,std::string> > Vid2Sid2Labels;
    /* Vid2Sid2Labels = {
        1 : {
            1001: uttid_1,
            1002: uttid_2,
            1003: uttid_3,
            },
        }
    */

    /* Hashes and LUT */
    std::unordered_map <size_t, std::unordered_map <int,std::set<int>>> Sid2Shash_DICT_DB;
    std::unordered_map <int, std::set<size_t>> LUT;

    bool InsertHashes(std::unordered_map <int, std::set<int>> &hashes, size_t Sid);
    bool EraseHashes(size_t Sid);
public:
    FastAudioShashManager();
    int64_t QueryProcess(std::unordered_map <int, std::set<int>> &hashes, size_t &Sid, uint64_t &db_vid, int &num_matches, int threshold_match);
    int64_t VerifyCanAddToDatabase(uint64_t Vid);
    int64_t AddDatabaseVideo(uint64_t Vid, size_t Sid, std::unordered_map <int, std::set<int>> &hashes );
    int64_t QueryVideo(std::unordered_map <int, std::set<int>> &hashes, uint64_t & db_vid, int &num_matches, int threshold_match, std::string &label_string);
    std::string DeleteVideo(uint64_t Vid);
    void DumpFile(std::string dir_path);
    void LoadFile(std::string dir_path);
    std::string Vid2Label(uint64_t Vid, size_t Sid);
    std::string InsertVidLabel(uint64_t Vid, size_t Sid, const std::string & label);

    void VerboseDatabase();
};
#endif // SHASH_FASTAUDIOSHASHMANAGER_H
