#include <iostream>
#include <sstream>
#include <string>
#include "shash/FastAudioShashManager.h"
#include "crow.h"
#include "MTSHasher.h"
#include "nlohmann/json.hpp"
#include <gflags/gflags.h>

DEFINE_string(service_name, "hello-shash-serv", "");

using json = nlohmann::json;

int main(int argc, char **argv){
    google::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << "Server version: 0.1.2" << std::endl;
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        exit(1);
    }
    MTSHasher hasher;

    std::cout << "Begin load conf..." << std::endl;
    hasher.init("/etc/shash", "/etc/shash_bak");

    /* testing */
        // printf("\n\n%s() : Performing deletion \n", __func__);
        // std::string tmp_label_string;
        // uint64_t tmp_cluster_id = 0;
        // int delret = hasher.del(1, tmp_label_string, tmp_cluster_id);
        // printf("%s() : delret = hasher.del(...) = %i \n", __func__, delret);
        // printf("%s() : tmp_label_string = %s \n", __func__, tmp_label_string.c_str());
        // printf("%s() : tmp_cluster_id   = %lu \n", __func__, tmp_cluster_id);

    crow::SimpleApp app;
    crow::mustache::set_base(".");


    printf("%s() : Compiling query_fun() ... \n", __func__);
    auto query_fun = [&](const crow::request& req){

        std::ostringstream os;
        int ret;
        std::string label_string="";
        uint64_t cluster_id = 0;
        int act=0, num_matches = 0;
        json r;
        r["status"] = -1;

        try {
            std::cout << "\n\nquery_fun() : === " << std::endl;
            // std::cout << "query_fun() : req.body = " << req.body << std::endl;
            auto x = crow::json::load(req.body);

            /* Main Details */
            uint64_t vid = (uint64_t) x["vid"].i();
            std::cout << "query_fun() : vid =" << vid << std::endl;
            uint64_t threshold_match = (uint64_t) x["threshold_match"].i();
            std::cout << "query_fun() : threshold_match =" << threshold_match << std::endl;
            std::string hash = x["hash"].s();
            std::cout << "query_fun() : \thash.size() =" << hash.size() << std::endl;

            /* Querying */
            int ret = hasher.query(vid, hash, label_string, cluster_id, num_matches, threshold_match);
            r["ret"] = ret;
            if (ret != 0){
                r["status"] = -3;
                r["error_string"] = "Error in hasher.query()";
            } else {
                r["status"] = 200;

                /* No matches found */
                if (cluster_id == 0) {
                    std::cout << "\t/* No matches found */" << std::endl;
                    r["act"] = 0;
                }

                /* Match found ! */
                else {
                    std::cout << "\t/* Match found ! */" << std::endl;
                    r["act"] = 1;
                    r["qry_vid"] = vid;
                    r["label_string"] = label_string;
                    r["db_vid"] = cluster_id;
                    r["num_matches"] = num_matches;
                }
            }

            std::cout << "query_fun() : Done" << std::endl;
        } catch (exception& e) {
            std::cout << "query_fun() : Error : " << e.what() << std::endl;
            r["error_string"] = e.what();
        }

        std::string retstr = r.dump();
        std::cout << "query_fun() : retstr = " << retstr << std::endl;
        return crow::response(retstr);
    };

    printf("%s() : Compiling del_fun() ... \n", __func__);
    auto del_fun = [&](const crow::request& req){

        int ret;
        std::string label_string="";
        uint64_t cluster_id = 0;
        json r;
        r["status"] = -1;

        try {
            std::cout << "\n\ndel_fun() : === " << std::endl;
            // std::cout << "del_fun() : req.body = " << req.body << std::endl;
            auto x = crow::json::load(req.body);

            /* Main Details */
            std::cout << "del_fun() : x[vid] =" << x["vid"] << std::endl;
            uint64_t vid = (uint64_t) x["vid"].i();
            r["vid"] = vid;
            std::cout << "del_fun\t vid            = " << vid << std::endl;

            /* Querying */
            ret = hasher.del(vid, label_string, cluster_id);
            r["ret"] = ret;
            if (ret != 0){
                r["status"] = -3;
                r["error_string"] = "Error in hasher.del()";
            } else {
                std::cout << "del_fun\t cluster_id     = " << cluster_id << std::endl;
                std::cout << "del_fun\t label_string   = " << label_string << std::endl;
                r["status"] = 200;
                r["cluster_id"] = cluster_id;
                r["label_string"] = label_string;
            }

            std::cout << "del_fun() : Done" << std::endl;
        } catch (exception& e) {
            std::cout << "del_fun() : Error : " << e.what() << std::endl;
            r["error_string"] = e.what();
        }

        std::string retstr = r.dump();
        std::cout << "del_fun() : retstr = " << retstr << std::endl;
        return crow::response(retstr);
    };

    printf("%s() : Compiling add_fun() ... \n", __func__);
    auto add_fun = [&](const crow::request& req){
        int ret;
        json r;
        r["status"] = -1;
        try {
            std::cout << "\n\n" << "add_fun() : === " << std::endl;
            // std::cout << "add_fun() : req.body = " << req.body << std::endl;

            auto x = crow::json::load(req.body);

            std::cout << "add_fun() : x[vid] =" << x["vid"] << std::endl;
            std::cout << "add_fun() : x[HashSize] =" << x["HashSize"] << std::endl;
            std::cout << "add_fun() : x[label] =" << x["label"] << std::endl;

            /* Main Details */
            uint64_t vid = (uint64_t) x["vid"].i();
            std::cout << "add_fun() : vid =" << vid << std::endl;

            int HashSize = x["HashSize"].i();
            std::cout << "add_fun() : HashSize =" << HashSize << std::endl;

            std::string label_string = x["label"].s();
            std::cout << "add_fun() : label_string =" << label_string << std::endl;

            /* Verify whether to add this vid */
            ret = hasher.checkadd(vid);
            if (ret != 0){
                r["ret"] = ret;
                r["status"] = -3;
                std::string vid_str = x["vid"].s();
                r["error_string"] = "vid = " + vid_str + " already exist in database, please use another vid.";
                // 
                std::string retstr = r.dump();
                std::cout << "add_fun() : retstr = " << retstr << std::endl;
                return crow::response(retstr);
            }

            /* Read shash and Adding */
            for (size_t i=1; i<=HashSize; i++) {
                std::cout << "add_fun() : i =" << i << std::endl;
                /* Get hash */
                size_t sid = vid*1000 + i;
                std::cout << "add_fun() : \tsid =" << sid << std::endl;

                std::cout << "add_fun() : \thash key =" << "hash"+std::to_string(i) << std::endl;
                std::string hash = x["hash"+std::to_string(i)].s();
                std::cout << "add_fun() : \thash.size() =" << hash.size() << std::endl;

                /* Adding */
                ret = hasher.add(sid, hash, label_string, vid);
                r["ret"] = ret;
                if (ret != 0){
                    r["status"] = -3;
                    r["error_string"] = "Error in hasher.add()";
                    // 
                    std::string retstr = r.dump();
                    std::cout << "add_fun() : retstr = " << retstr << std::endl;
                    return crow::response(retstr);
                } else {
                    r["status"] = 200;
                    r["label_string"] = label_string;
                }
                std::cout << "add_fun() : \tret =" << ret << std::endl;
            }

            std::cout << "add_fun() : Done" << std::endl;
        } catch (exception& e) {
            std::cout << "add_fun() : Error : " << e.what() << std::endl;
            r["error_string"] = e.what();
        }

        std::string retstr = r.dump();
        std::cout << "add_fun() : retstr = " << retstr << std::endl;
        return crow::response(retstr);
    };

    printf("%s() : Compiling dump_fun() ... \n", __func__);
    auto dump_fun = [&](const crow::request& req){

        int ret;
        json r;
        r["status"] = -1;

        try {
            std::cout << "\n\ndump_fun() : === " << std::endl;

            /* Dumping */
            std::cout << "dump_fun() : hasher.dump() " << std::endl;
            ret = hasher.dump();
            std::cout << "dump_fun() : r[ret]=" << ret << std::endl;
            r["ret"] = ret;
            if (ret != 0){
                r["status"] = -3;
                r["error_string"] = "Error in hasher.dump()";
            } else {
                r["status"] = 200;
            }

            std::cout << "dump_fun() : Done" << std::endl;
        } catch (exception& e) {
            std::cout << "dump_fun() : Error : " << e.what() << std::endl;
            r["error_string"] = e.what();
        }

        std::string retstr = r.dump();
        std::cout << "dump_fun() : retstr = " << retstr << std::endl;
        return crow::response(retstr);
    };

    printf("%s() : Compiling ping_fun() ... \n", __func__);
    auto ping_fun = [&](const crow::request& req){
        std::cout << "dump_fun() : === " << std::endl;
        return crow::response("pong");
    };
    CROW_ROUTE(app, "/api/ping").methods("POST"_method)(ping_fun);
    CROW_ROUTE(app, "/api/ping")(ping_fun);


    CROW_ROUTE(app, "/api/hello-shash-serv/query").methods("POST"_method)(query_fun);
    CROW_ROUTE(app, "/api/hello-shash-serv/query")(query_fun);

    CROW_ROUTE(app, "/api/hello-shash-serv/del").methods("POST"_method)(del_fun);
    CROW_ROUTE(app, "/api/hello-shash-serv/del")(del_fun);

    CROW_ROUTE(app, "/api/hello-shash-serv/add").methods("POST"_method)(add_fun);
    CROW_ROUTE(app, "/api/hello-shash-serv/add")(add_fun);

    CROW_ROUTE(app, "/api/hello-shash-serv/dump").methods("POST"_method)(dump_fun);
    CROW_ROUTE(app, "/api/hello-shash-serv/dump")(dump_fun);

    app.loglevel(crow::LogLevel::INFO);
    std::cout << "Begin server. Port: " << argv[1] << std::endl;
    std::cout << "service_name: " << FLAGS_service_name << std::endl;
  
    app.port(std::stoi(argv[1]))
        .multithreaded()
        .run();
}
