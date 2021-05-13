#include "Shash.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include "FastAudioShashManager.h"
#include <stdio.h>
#include <sys/stat.h>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

std::string encode_hash(const std::unordered_map <int, std::set<int>>& hash) {
    json j_map_4(hash);
    return j_map_4.dump();
}
std::unordered_map <int, std::set<int>> decode_hash(std::string hash_str) {
    json shash_json = json::parse(hash_str);
    return shash_json.get<std::unordered_map <int, std::set<int>>>();
}

FastAudioShashManager::FastAudioShashManager() {
}

bool FastAudioShashManager::InsertHashes(
    std::unordered_map <int, std::set<int>> &hashes,
    size_t Sid) {
    /* This updates Sid2Shash_DICT_DB & LUT.
    Sid2Shash_DICT_DB stores the Sid's hashes.
    LUT is used to search for db_hash when querying.

        Inputs:
        -------
        hashes : std::unordered_map <int, std::set<int>> &
            e.g. hash = [ 3:[1,2,3], 4:[1,2,6] ]

        Sid : size_t
            this is the LUT's key for the matched db.
            Also the index of the db in the list.
    */

    /* Update Sid2Shash_DICT_DB */
        Sid2Shash_DICT_DB[Sid] = hashes;

    /* Update LUT (for queue system) */
        addLUT(LUT, hashes, Sid);

    return true;
}

bool FastAudioShashManager::EraseHashes(size_t Sid) {
    /* Removes Sid and its hash from Sid2Shash_DICT_DB & LUT.
    so next time queries dont search for this anymore.

        Inputs:
        -------
        Sid : size_t
            this is the LUT's key for the matched db.
            Also the index of the db in the list.
    */

    /* LUT */
        printf("\t%s(): LUT.size() = %lu (bfore) \n", __func__, LUT.size());
        eraseLUT(LUT, Sid2Shash_DICT_DB[Sid], Sid);
        printf("\t%s(): LUT.size() = %lu (after) \n", __func__, LUT.size());

    /* Sid2Shash_DICT_DB */
    if (Sid2Shash_DICT_DB.count(Sid) > 0){
        Sid2Shash_DICT_DB.erase(Sid);
    } else {
        printf("\t%s(): ERROR - Sid %lu does not exist\n", __func__, Sid);
    }

    return true;
}

int64_t FastAudioShashManager::QueryProcess(
    std::unordered_map <int, std::set<int>> &qry_shash,
    size_t &Sid,
    uint64_t &db_vid,
    int &num_matches,
    int threshold_match
    ) {
    /* Performs the querying against the database

        Inputs:
        -------
        qry_shash : std::unordered_map <int, std::set<int>> &
            e.g. hash = [ 3:[1,2,3], 4:[1,2,6] ]

        threshold_match : int
            For data_insert == 1.
            if num_matches exceeds this threshold, it is deemed as a match.

        Returns:
        -------
        Sid : size_t
            this is the LUT's key for the matched db.
            Also the index of the db in the list.

        db_vid : uint64_t
            db_vid of the matched db.

        shash_decision : int64_t
            1 if there exists a match.
            0 if there are no matches found.

        num_matches : int
            For data_insert == 1.
            The number of matches between query and the matched db.
    */

    int bins=23, shash_decision=0;

    std::vector<size_t> CandidateSet = searchLUT_v2(LUT, qry_shash, 3); // set TopN={2 or 3};
    printf("\t%s() : CandidateSet.size() = %lu \n", __func__, CandidateSet.size());
    for (const auto &curr_sid : CandidateSet){
        std::unordered_map <int, std::set<int>>  db_shash = Sid2Shash_DICT_DB[curr_sid];
        shash_decision = shashmatching_v2(db_shash, qry_shash, num_matches, threshold_match, bins);
        printf("\t%s() : qrying Sid=%lu | num_matches=%i | threshold_match=%i \n", __func__, curr_sid, num_matches, threshold_match);

        /* it is a match */
        if (shash_decision == 1) {
            Sid = curr_sid;
            db_vid = Sid2Vid[Sid];
            printf("\t%s() : Match found ! Sid=%lu db_vid=%lu label=%s \n", __func__, 
                Sid,
                db_vid,
                Vid2Sid2Labels[db_vid][Sid].c_str());
            return shash_decision;
        }
    }
    /* There is no match */
    printf("\t%s() : no match found. \n", __func__);
    return shash_decision;
}


int64_t FastAudioShashManager::VerifyCanAddToDatabase(uint64_t Vid) {
    /* Checks whether vid already exists inside.

        Inputs:
        -------
        Vid : uint64_t
            input db_vid

        Returns:
        --------
        0 : if does not exists, then we can proceed to adding the shash.
        1 : Already exists, either you are submitting a duplicate, or you need to change to another unique vid
    */

    /* update Vid <-> Sid */
    if (Vid2Sid2Labels.count(Vid) > 0) {
        return 1;
    }

    return 0;
}


int64_t FastAudioShashManager::AddDatabaseVideo(
    uint64_t Vid,
    size_t Sid,
    std::unordered_map <int, std::set<int>> &hashes
    ) {
    /* Adding video and its shash to the database


        Inputs:
        -------

        Vid : uint64_t
            input db_vid

        hash : std::unordered_map <int, std::set<int>> &
            db's hash to add into the database
            e.g. hash = [   3:[1,2,3],  4:[1,2,6]   ]


        Returns:
        --------
        cluster_id : int64_t
            cluster_id the Vid
    */

    /* update Vid <-> Sid */
    Sid2Vid[Sid] = Vid;

    /* update Sid2Shash_DICT_DB & LUT */
    InsertHashes(hashes, Sid);

    return (int64_t) Vid;
}
int64_t FastAudioShashManager::QueryVideo(
    std::unordered_map <int, std::set<int>> &hashes,
    uint64_t &db_vid,
    int &num_matches,
    int threshold_match,
    std::string &label_string
    ) {
    /* Adding video and its shash to the database 

        Inputs:
        -------

        hash    : std::unordered_map <int, std::set<int>> &
            e.g. hash = [3:[1,2,3],4:[1,2,6]]

        data_insert : int
            Whether or not to insert into LUT.
            0 if taking in qry shash, perform querying.
            1 if taking in db shash, updating LUT.
            2 if ?
            3 if ?

        db_vid  : uint64_t
            output Vid, db_vid or 0
            if data_insert == 0, this is db_vid that matched to the input Vid, if no match, just 0
            if data_insert == 1, this is just the input Vid

        num_matches : int
            if data_insert == 1,
            the number of matches between query and the matched db
    */

    size_t Sid;
    int64_t Final_Db_Vid = 0;
    db_vid = 0;
    label_string = "";

    /* data_insert == 0 : perform querying */
    printf("\t%s() : Perform querying ... \n", __func__);
    int shash_decision = QueryProcess(hashes, Sid, db_vid, num_matches, threshold_match);
    printf("\t%s() : shash_decision = %i \n", __func__, shash_decision);
    if (shash_decision == 1){
        Final_Db_Vid = (int64_t) Sid2Vid[ Sid ];
        label_string = Vid2Label(Final_Db_Vid, Sid);
        printf("\t%s() : Sid  = %lu \n", __func__, Sid);
        printf("\t%s() : Final_Db_Vid   = %lu \n", __func__, Final_Db_Vid);
        printf("\t%s() : db_vid         = %lu \n", __func__, db_vid);
        printf("\t%s() : label_string   = %s \n", __func__, label_string.c_str());
    }
    return Final_Db_Vid;
}


void FastAudioShashManager::VerboseDatabase() {
    printf("\t%s() : -------- \n", __func__);
    int idx;

    /* Vid <-> Sid */
        printf("\t%s() : Sid2Vid (size=%lu) = \n", __func__, Sid2Vid.size());
            idx=0;
            for (const auto &it : Sid2Vid){
                printf("\t\tSid2Vid[%lu]=%lu\n", it.first, it.second);
                idx++;
                if (idx>=3) {break;}
            }
        printf("\t%s() : Vid2Sid2Labels (size=%lu) = \n", __func__, Vid2Sid2Labels.size());
            idx=0;
            for (const auto &it:Vid2Sid2Labels) {
                printf("\t%s() : \t\tVid2Sid2Labels[%lu] = (has size=%lu) {", __func__, it.first, it.second.size());
                int jdx=0;
                for (const auto &it1: it.second) {
                    printf("\t[%lu]=%s,", it1.first, it1.second.c_str() );
                    jdx++;
                    if (jdx>=3) {break;}
                }
                printf("}\n");
                idx++;
                if (idx>=3) {break;}
            }

    /* LUT */
        printf("\t%s() : LUT (size=%lu) = \n", __func__, LUT.size());
    /* Sid2Shash_DICT_DB */
        idx=0;
        printf("\t%s() : Sid2Shash_DICT_DB (size=%lu) = \n", __func__, Sid2Shash_DICT_DB.size());
        for (const auto &it:Sid2Shash_DICT_DB) {
            printf("\t%s() : \tutt2shash_DICT_DB[%lu] = has hash with %lu keys \n", __func__,
                it.first,
                it.second.size());
            idx++;
            if (idx>=3) {break;}
        }

    printf("\t%s() : -------- \n", __func__);
}

std::string FastAudioShashManager::DeleteVideo(uint64_t Vid) {
    /* Deletes Sid's hashes associated with the Vid for the desired video.

        Inputs:
        -------
        Vid : uint64_t
            desired video id to be removed.

    */
    std::string label_str;

    if (Vid2Sid2Labels.count(Vid) > 0) {
        std::unordered_map <size_t,std::string> &Sid2Labels = Vid2Sid2Labels[Vid];

        /* Delete its hash. */
        for (auto &it : Sid2Labels){
            size_t Sid = it.first;
            EraseHashes(Sid);
            Sid2Vid.erase(Sid);
            label_str = Sid2Labels[Sid];
            printf("\t%s() : Removed Sid = %lu \n", __func__, Sid);
        }

        Vid2Sid2Labels.erase(Vid);
    }

    return label_str;
}


void FastAudioShashManager::DumpFile(std::string dir_path) {
    /* Writes all the hashes and clusters to a local file 
        dir_path/Info:
            <Number Of Hashes>
            --
            <Sid_1>
            <Vid_1>
            <Hash Length for this Sid>
            <Hash for this Sid ...>
            --
            <Sid_2>
            <Vid_2>
            <Hash Length for this Sid>
            <Hash for this Sid ...>
            --
        
        dir_path/Label:
            <Number Of Clusters>
            --
            <Vid_1>,<Number of Sid in this cluster>
            <Sid_1>,<Label for this Sid_1>
            <Sid_2>,<Label for this Sid_2>
            --
            <Vid_2>,<Number of Sid in this cluster>
            <Sid_3>,<Label for this Sid_3>
            <Sid_4>,<Label for this Sid_4>
            --
    */
    printf("\t%s() : ---------\n", __func__);
    /* Creating Directory */
        printf("\t%s() : Creating Directory \n", __func__);
        dir_path += '/';
        mkdir(dir_path.c_str(), 0777);

    /* Open shash/Info */
        printf("\t%s() : Open shash/Info \n", __func__);
        FILE *fout = fopen((dir_path + "Info").c_str(), "w");
        if (!fout) {
            std::cerr<<"Dump Info File ERROR, please make sure "<< dir_path << " exists"<<std::endl;
            return;
        }
    /* Print Sid2Shash_DICT_DB details */
        printf("\t%s() : Sid2Shash_DICT_DB.size() = %lu\n", __func__, Sid2Shash_DICT_DB.size());
        fprintf(fout, "%lu\n", Sid2Shash_DICT_DB.size());
        for (const auto &it : Sid2Vid) {

            /* Init */
            size_t  Sid = it.first;
            uint64_t Vid = it.second;

            /* Writing Vid details */
            fprintf(fout, "%lu\n", Sid);
            fprintf(fout, "%lu\n", Vid);

            /* Writing hash details */
            std::string shash_str = encode_hash(Sid2Shash_DICT_DB[Sid]);
            fprintf(fout, "%lu\n", shash_str.size());
            fprintf(fout, "%s\n", shash_str.c_str());

        }
        fclose(fout);
        printf("\t%s() : Dump File to %sInfo SUCCESS\n", __func__, dir_path.c_str());

    /* Open shash/Labels */
        printf("\t%s() : Open shash/Labels \n", __func__);
        fout = fopen((dir_path + "Labels").c_str(), "w");
        if (!fout) {
            perror("Dump Label File ERROR");
            return;
        }
    /* Write Vid2Sid2Labels */
        printf("\t%s() : Vid2Sid2Labels.size() = %lu\n", __func__, Vid2Sid2Labels.size());
        fprintf(fout, "%zd\n", Vid2Sid2Labels.size());
        for (auto & Sid2Labels : Vid2Sid2Labels) {
            fprintf(fout, "%lu,%lu\n", Sid2Labels.first, Sid2Labels.second.size());
            for (auto & sid_label_c : Sid2Labels.second) {
                fprintf(fout, "%lu,%s\n", sid_label_c.first, sid_label_c.second.c_str());
            }
        }
        fclose(fout);
        printf("\t%s() : Dump File to %sLabels SUCCESS\n", __func__, dir_path.c_str());

    printf("\t%s() : ---------\n", __func__);
}
void FastAudioShashManager::LoadFile(std::string dir_path) {
    /* Reads all the hashes and clusters from a local file.
        See DumpFile() for details.
    */
    printf("\t%s() : ---------\n", __func__);
    /* Loading Labels */
    /* Init */
        Sid2Vid.clear();
        Vid2Sid2Labels.clear();
        std::string l;
        size_t Sid = 0;
        uint64_t Vid = 0;
        uint64_t num_vids = 0;
        std::string cid_str;
        std::string label_str;
        std::string num_vids_str;
        std::ifstream ifs(dir_path+"/Labels");
    /* Open */
        if(!ifs.is_open()) {
            std::cerr<<" Warning, open "<<(dir_path+"/Labels failed")<<std::endl;
            std::cerr<<"   Vid2Label() cannot be used"<<std::endl;
            return;
        }
    /* Reading */
        uint64_t total_cids;
        std::getline(ifs, l);
        std::stringstream firstss(l);
        firstss >> total_cids;
        printf("\t%s() : total_cids:%zd\n", __func__, total_cids);
        for (size_t i = 0; i < total_cids; i++) {
            l.clear();
            std::getline(ifs, l);
            if (l.empty()) {
                break;
            }
            Vid = 0;
            num_vids = 0;
            num_vids_str.clear();
            cid_str.clear();
            std::stringstream ss(l);
            ss >> Vid;
            // printf("\t%s() : ss >> Vid = %lu \n", __func__, Vid);
            if(ss.peek() == ',') ss.ignore();
            ss >> num_vids;
            std::unordered_map <size_t,std::string> currSid2Labels;
            for (auto x=0u; x<num_vids; x++) {
                std::getline(ifs, l);
                if(l.empty()) {
                    break;
                }
                std::string lbl;
                std::istringstream iss(l);
                iss >> Sid;
                if (iss.peek() == ',') iss.ignore();
                std::getline(iss, lbl, '\n');
                currSid2Labels[Sid] = lbl;
                Sid2Vid[Sid] = Vid;
                // printf("\t%s() : Sid2Vid[%lu] = %lu \n", __func__, Sid, Vid);
            }
            Vid2Sid2Labels[Vid] = currSid2Labels;
            // printf("\t%s() : Vid2Sid2Labels[%lu] = currSid2Labels \n", __func__, Vid);
        }
        ifs.close();
        if (Vid2Sid2Labels.size() == total_cids) {
            printf("\t%s() : Load Label SUCCESS, %zd cids loaded.\n", __func__, total_cids);
        } else {
            printf("\t%s() : Load Label ERROR, need %zd cids, but loaded %zd\n", __func__, total_cids, Vid2Sid2Labels.size());
        }


    /** Read from Info **/
    /* Open Info file */
        printf("\t%s() : Open Info file \n", __func__);
        FILE *fin = fopen((dir_path + "/Info").c_str(), "r");
        if (!fin) {
            perror("Load File ERROR");
            return;
        }
    /* start reading hash & Vid */
        printf("\t%s() : start reading hash & Vid \n", __func__);
        size_t NumberOfSids;
        int ret = fscanf(fin, "%lu", &NumberOfSids);
        for (size_t i = 0; i < NumberOfSids; ++ i) {
        /* read Vid & Sid */
            uint64_t Vid;
            size_t Sid;
            ret = fscanf(fin, "%lu", &Sid);
            ret = fscanf(fin, "%lu", &Vid);
            // printf("\t%s() : Sid = %lu\n", __func__, Sid);
            // printf("\t%s() : Vid = %lu\n", __func__, Vid);
        /* read the hash */
            size_t hashes_size;
            ret = fscanf(fin, "%lu", &hashes_size);
            // printf("\t%s() : hashes_size = %lu\n", __func__, hashes_size);
            char hashes_char[hashes_size];
            ret = fscanf(fin, "%s", hashes_char );
            std::string hashes_str(hashes_char);
            // assert(hashes_str == hashes_char);
            std::unordered_map <int, std::set<int>> hashes = decode_hash(hashes_str);
        /* Update LUT & Sid2Shash_DICT_DB */
            // printf("\t%s() : vids_size = %lu\n", __func__, vids_size);
            InsertHashes(hashes, Sid);
        }
        fclose(fin);
        printf("\t%s() : Load Info SUCCESS, %zd cids loaded\n", __func__, Vid2Sid2Labels.size());

    printf("\t%s() : ---------\n", __func__);
}


std::string FastAudioShashManager::Vid2Label(uint64_t Vid, size_t Sid) {
    std::string label_str;
    if (Vid2Sid2Labels.count(Vid) > 0) {
        std::unordered_map <size_t, std::string> &Sid2Labels = Vid2Sid2Labels[Vid];
        if (Sid2Labels.count(Sid) > 0) {
            label_str = Sid2Labels[Sid];
        }
    }
    return label_str;
}

std::string FastAudioShashManager::InsertVidLabel(
    uint64_t Vid,
    size_t Sid,
    const std::string & label) {
    /* 

        Sid2Vid = {
            1001 : 1,
            1002 : 1,
            1003 : 1,
            }

        Vid2Sid2Labels = {
            1 : {
                1001: uttid_1,
                1002: uttid_2,
                1003: uttid_3,
                },
            }
    */

    std::string ret_labels_str;
    printf("\t%s() : Vid   = %lu \n", __func__, Vid);
    printf("\t%s() : Sid   = %lu \n", __func__, Sid);
    printf("\t%s() : label = %s \n", __func__, label.c_str());

    if (Vid2Sid2Labels.count(Vid) > 0) {
        std::unordered_map <uint64_t,std::string> &Sid2Labels = Vid2Sid2Labels[Vid];
        if (Sid2Labels.count(Sid) > 0) {
            ret_labels_str = Sid2Labels[Sid];
        }
        Sid2Labels[Sid] = label;
    } else {
        std::unordered_map <uint64_t,std::string> Sid2Labels;
        Sid2Labels[Sid] = label;
        Vid2Sid2Labels[Vid] = Sid2Labels;
    }

    return ret_labels_str;
}
