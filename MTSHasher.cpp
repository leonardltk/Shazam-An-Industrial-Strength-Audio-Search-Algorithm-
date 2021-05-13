#include "MTSHasher.h"
#include "shash/FastAudioShashManager.h"

//Multi-threading SHash query support
MTSHasher::MTSHasher(){}
MTSHasher::~MTSHasher(){}

void MTSHasher::init(const std::string & _ban_lib_dir, const std::string & _bak_lib_dir) {
    wlock.lock();
    solvers[0].LoadFile(_ban_lib_dir);
    solvers[0].DumpFile(_bak_lib_dir);
    solvers[1].LoadFile(_bak_lib_dir);
    solvers[1].VerboseDatabase();
    ban_lib_dirs[0] = _ban_lib_dir;
    ban_lib_dirs[1] = _bak_lib_dir;
    wlock.unlock();
}


int MTSHasher::checkadd(
    uint64_t & db_vid){
    /* Checks if this vid is already added

        Inputs:
        -------
        db_vid  : uint64_t &
            cluster of the input db_vid's hash

        Returns:
        -------
        ret         : int
            whether or not this runs successfully
    */

    /* Convert hash (string --> map)*/
    wlock.lock();

    /* check if can add to solver[1] (backup) */
        curr_solver_idx = 1;
        while (ongoing_query[curr_solver_idx] > 0) {}; //busy waiting

        if ( 0 != solvers[curr_solver_idx].VerifyCanAddToDatabase(db_vid) ){
            std::cout << __func__ << "() : db_vid = " << db_vid
                      << " already exist in database..." << std::endl;
            wlock.unlock();
            return 1;
        }

    /* check if can add to solver[0] (main) */
        curr_solver_idx = 0;
        while (ongoing_query[curr_solver_idx] > 0) {};

        if ( 0 != solvers[curr_solver_idx].VerifyCanAddToDatabase(db_vid) ){
            std::cout << __func__ << "() : db_vid = " << db_vid
                      << " already exist in database..." << std::endl;
            wlock.unlock();
            return 1;
        }

    wlock.unlock();
    return 0;
}
int MTSHasher::add(
    uint64_t vid,
    const std::string & hash,
    const std::string & label_string,
    uint64_t & db_vid){
    /* Perform adding of shash to update current database

        Inputs:
        -------
        vid             : uint64_t
            input db_vid

        hash            : const std::string &
            encoded hash in string format
            e.g. hash = [3:[1,2,3],4:[1,2,6]]

        label_string    : const std::string &
            Some label from the input

        db_vid  : uint64_t &
            cluster of the input db_vid's hash

        Returns:
        -------
        ret         : int
            whether or not this runs successfully
    */

    /* Convert hash (string --> map)*/
    wlock.lock();
    auto && hashes = decode_hash(hash);

    /* Perform AddVideo() InsertCidLabel() on solver[1] (backup) */
        curr_solver_idx = 1;
        while (ongoing_query[curr_solver_idx] > 0) {}; //busy waiting

        auto db_vid_0 = (uint64_t) solvers[curr_solver_idx].AddDatabaseVideo(db_vid, vid, hashes);
        auto && ret_str = solvers[curr_solver_idx].InsertVidLabel(db_vid, vid, label_string);

        if (ret_str.length() > 0) {
            std::cout << __func__ << "() : replacing " << ret_str << " with " << label_string << std::endl;
        }

    /* Perform AddVideo() InsertCidLabel() on solver[0] (main) */
        curr_solver_idx = 0;
        while (ongoing_query[curr_solver_idx] > 0) {};

        auto db_vid_1 = (uint64_t) solvers[curr_solver_idx].AddDatabaseVideo(db_vid, vid, hashes);
        solvers[curr_solver_idx].InsertVidLabel(db_vid, vid, label_string); // InsertCidLabel(cluster_id_1, vid, label_string);

        if(db_vid_0 != db_vid_1) {
            std::cerr << "add cluster id " << db_vid_0 << " vs " << db_vid_1
                      << "does not agree! inner error, exiting" << std::endl;
            wlock.unlock();
            exit(-9);
        }

    db_vid = db_vid_0;
    solvers[0].VerboseDatabase();
    solvers[1].VerboseDatabase();

    wlock.unlock();
    return 0;
}

int MTSHasher::del(
    uint64_t vid,
    std::string & label_string,
    uint64_t & cluster_id){

    wlock.lock();
    label_string = "";

    /* Perform AddVideo() InsertCidLabel() on solver[1] (backup) */
        curr_solver_idx = 1;
        while (ongoing_query[0] > 0) {}; //busy waiting
        std::string label_string_0 = solvers[0].DeleteVideo(vid);
        std::cout << __func__ << "() : label_string_0 = solvers[0].DeleteVideo(vid) = " << label_string_0 << std::endl;

    /* Perform AddVideo() InsertCidLabel() on solver[0] (main) */
        curr_solver_idx = 0;
        while (ongoing_query[1] > 0) {};
        std::string label_string_1 = solvers[1].DeleteVideo(vid);
        std::cout << __func__ << "() : label_string_1 = solvers[1].DeleteVideo(vid) = " << label_string_1 << std::endl;

    /* Sanity Check */
        if (label_string_0.compare(label_string_1) != 0) {
            std::cerr << "del label_string " << label_string_0 << " vs " << label_string_1
                      << "does not agree! inner error, exiting" << std::endl;
            wlock.unlock();
            exit(-10);
        }

    label_string = label_string_0;
    cluster_id = (uint64_t) vid;
    solvers[0].VerboseDatabase();
    solvers[1].VerboseDatabase();

    wlock.unlock();
    if (cluster_id <= 0) {
         return -1;
    }

    return 0;
}

int MTSHasher::query(
    uint64_t vid,
    const std::string &hash,
    std::string &label_string,
    uint64_t &db_vid,
    int &num_matches,
    int threshold_match) {
    wlock.lock();

    int curr_idx = curr_solver_idx;
    ongoing_query[curr_idx] += 1;
    auto && hashes = decode_hash(hash);

    db_vid = (uint64_t) solvers[curr_idx].QueryVideo(hashes, db_vid, num_matches, threshold_match, label_string);

    ongoing_query[curr_idx] -= 1;

    // solvers[curr_idx].VerboseDatabase();
    wlock.unlock();
    return 0;
}


int MTSHasher::dump() {
    wlock.lock();
    std::cout << __func__ << "() : solvers[0].DumpFile(ban_lib_dirs[0]); " << std::endl;
    solvers[0].DumpFile(ban_lib_dirs[0]);
    std::cout << __func__ << "() : solvers[1].DumpFile(ban_lib_dirs[1]); " << std::endl;
    solvers[1].DumpFile(ban_lib_dirs[1]);
    wlock.unlock();
    return 0;
}
