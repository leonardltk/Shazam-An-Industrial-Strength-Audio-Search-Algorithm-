#ifndef AUDIO_MTSHASHER_H
#define AUDIO_MTSHASHER_H

#include "shash/FastAudioShashManager.h"
#include <mutex>

#include <sstream>
#include <iostream>
#include <map>
#include <mutex>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

using namespace std;

/* Multi-threading SHash query support */
class MTSHasher {
public:
    MTSHasher();
    ~MTSHasher();

    void init(const std::string & lib_dir, const std::string & bak_lib_dir);
    int query(uint64_t vid, const std::string &hash, std::string & label_string, uint64_t &cluster_id, int &num_matches, int threshold_match);

    int checkadd(uint64_t & db_vid);
    int add(uint64_t vid, const std::string & hash, const std::string & label_string, uint64_t & cluster_id);

    int del(uint64_t vid, std::string & label_string, uint64_t & cluster_id);
    int dump();

private:
    FastAudioShashManager solvers[2];
    volatile int curr_solver_idx = 0;
    std::mutex wlock;
    std::string ban_lib_dirs[2];
    volatile uint32_t ongoing_query[2] = {0, 0};
};


#endif //AUDIO_MTSHASHER_H
