#pragma once
#include <audio_feature.pb.h>
#include <brpc/channel.h>
#include <butil/iobuf.h>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

DEFINE_string(audio_db_url, "103.240.149.46:80", "");
DEFINE_string(audio_db_uri, "/api/hello-shash-serv/", "");
DEFINE_int32(audio_db_timeout_ms, 30000, "");
DEFINE_int32(audio_db_max_retry, 0, "");

class AudioDbClient {
public:
    AudioDbClient() {
        brpc::ChannelOptions options;
        options.protocol = "http";
        options.timeout_ms = FLAGS_audio_db_timeout_ms;
        options.max_retry = FLAGS_audio_db_max_retry;
        LOG(INFO) << "the query audio_db url: " << FLAGS_audio_db_url;
        LOG(INFO) << "the query audio_db uri: " << FLAGS_audio_db_uri;
        LOG_IF(FATAL, channel_.Init(FLAGS_audio_db_url.c_str(), &options)) << "Failed to init channel to ann database with url: " << FLAGS_audio_db_url;
    }

    int http_get_write(brpc::Controller* cntl,
        const std::string& vid,
        const std::vector< std::unordered_map <int, std::set<int>> > &hashes,
        const std::string& label) {
        LOG(INFO) << __FUNCTION__ << ": ================== ";
        LOG(INFO) << __FUNCTION__ << ": vid = " << vid;
        LOG(INFO) << __FUNCTION__ << ": hashes has length = " << hashes.size();

        if (hashes.size() == 0) {
            cntl->http_response().set_status_code(500);
            cntl->response_attachment().append("hashes is empty, vid="+vid);
            return -1;
        }

        brpc::Controller db_cntl;
        /* Request URL */
        db_cntl.http_request().uri() = FLAGS_audio_db_uri + "add";
        /* POST instead of GET */
        db_cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        /* content to transfer over */
        std::string sending_s("{");
        sending_s.append("\"vid\":\"" + vid + "\",");
        sending_s.append("\"HashSize\":\"" + std::to_string(hashes.size()) + "\",");
        for (size_t i=1; i<=hashes.size(); i++) {
            LOG(INFO) << __FUNCTION__ << ": appending hash index = " << i;
            sending_s.append("\"hash"+ std::to_string(i) +"\":\"" + encode_hash(hashes[i-1]) + "\",");
        }
        sending_s.append("\"label\":\"" + label + "\"");
        sending_s.append("}");
        db_cntl.request_attachment().append(sending_s);

        LOG(INFO) << __FUNCTION__ << ": appending vid = " << vid;
        LOG(INFO) << __FUNCTION__ << ": appending label = " << label;
        // LOG(INFO) << __FUNCTION__ << ": sending_s: " << sending_s;

        LOG(INFO) << __FUNCTION__ << ": FLAGS_audio_db_uri = " << FLAGS_audio_db_uri;
        LOG(INFO) << __FUNCTION__ << ": db_cntl.http_request().uri() = " << db_cntl.http_request().uri();
        // LOG(INFO) << __FUNCTION__ << ": db_cntl.request_attachment() = " << db_cntl.request_attachment();

        LOG(INFO) << __FUNCTION__ << ": channel_.CallMethod()";
        channel_.CallMethod(NULL, &db_cntl, NULL, NULL, NULL);

        if (db_cntl.Failed()) {
            LOG(ERROR) << __FUNCTION__ << ": !!! ERROR !!! " << db_cntl.ErrorText();
            cntl->http_response().set_status_code(503);
            cntl->response_attachment().append(db_cntl.ErrorText());
            return -1;
        }

        LOG(INFO) << __FUNCTION__ << ": db_cntl.response_attachment().to_string() ";
        LOG(INFO) << __FUNCTION__ << ": " << db_cntl.response_attachment().to_string();
        cntl->response_attachment() = db_cntl.response_attachment();
        return 0;
    }

    int http_get_query(
        brpc::Controller* cntl,
        const std::unordered_map <int, std::set<int>>& hash,
        const std::string& vid,
        const std::string& threshold_match) {
        LOG(INFO) << __FUNCTION__ << ": vid = " << vid << " | hash.size() = " << hash.size() << " | threshold_match = " << threshold_match;

        if (hash.size() == 0) {
            cntl->http_response().set_status_code(500);
            cntl->response_attachment().append("hash is empty");
            return -1;
        }

        std::string threshold_match_toserv;
        if (threshold_match.compare("") == 0) {
            LOG(INFO) << __FUNCTION__ << ": threshold_match = " << threshold_match << " was not initialised, setting to 109";
            threshold_match_toserv = "109";
        } else {
            LOG(INFO) << __FUNCTION__ << ": threshold_match = " << threshold_match << " was initialised";
            threshold_match_toserv = threshold_match;
        }

        brpc::Controller db_cntl;
        /* Request URL */
        db_cntl.http_request().uri() = FLAGS_audio_db_uri + "query";
        /* POST instead of GET */
        db_cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        /* content to transfer over */
        std::string sending_s("{");
        sending_s.append("\"vid\":\"" + vid + "\",");
        sending_s.append("\"threshold_match\":\"" + threshold_match_toserv + "\",");
        sending_s.append("\"hash\":\"" + encode_hash(hash) + "\"");
        sending_s.append("}");
        db_cntl.request_attachment().append(sending_s);

        channel_.CallMethod(NULL, &db_cntl, NULL, NULL, NULL);
        if (db_cntl.Failed()) {
            LOG(ERROR) << __FUNCTION__ << ": " << db_cntl.ErrorText();
            cntl->http_response().set_status_code(503);
            cntl->response_attachment().append(db_cntl.ErrorText());
            LOG(ERROR) << __FUNCTION__ << ": db_cntl.response_attachment() = " << db_cntl.response_attachment().to_string();
            return -1;
        }

        LOG(INFO) << __FUNCTION__ << ": db_cntl.response_attachment() = " << db_cntl.response_attachment().to_string();
        cntl->response_attachment() = db_cntl.response_attachment();

        return 0;
    }

    int http_get_del(
        brpc::Controller* cntl,
        const std::string& vid) {
        LOG(INFO) << __FUNCTION__ << ": ================== ";
        LOG(INFO) << __FUNCTION__ << ": vid = " << vid;

        brpc::Controller db_cntl;
        /* Request URL */
        db_cntl.http_request().uri() = FLAGS_audio_db_uri + "del";
        /* POST instead of GET */
        db_cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        /* content to transfer over */
        std::string sending_s("{\"vid\":\"" + vid + "\"}");
        db_cntl.request_attachment().append(sending_s);

        LOG(INFO) << __FUNCTION__ << ": appending vid = " << vid;
        LOG(INFO) << __FUNCTION__ << ": FLAGS_audio_db_uri = " << FLAGS_audio_db_uri;
        LOG(INFO) << __FUNCTION__ << ": db_cntl.http_request().uri() = " << db_cntl.http_request().uri();
        LOG(INFO) << __FUNCTION__ << ": db_cntl.request_attachment() = " << db_cntl.request_attachment();

        LOG(INFO) << __FUNCTION__ << ": channel_.CallMethod()";
        channel_.CallMethod(NULL, &db_cntl, NULL, NULL, NULL);
        if (db_cntl.Failed()) {
            LOG(ERROR) << __FUNCTION__ << ": " << db_cntl.ErrorText();
            cntl->http_response().set_status_code(503);
            cntl->response_attachment().append(db_cntl.ErrorText());
            return -1;
        }

        LOG(INFO) << db_cntl.response_attachment().to_string();
        cntl->response_attachment() = db_cntl.response_attachment();

        return 0;
    }

    int http_get_displayStats(
        brpc::Controller* cntl) {
        LOG(INFO) << __FUNCTION__ << ": ================== ";

        brpc::Controller db_cntl;
        std::string sending_s(FLAGS_audio_db_uri + "displayStats?vid=");
        sending_s.append("&hash=");
        db_cntl.http_request().uri() = sending_s;
        db_cntl.http_request().set_method(brpc::HTTP_METHOD_GET);

        channel_.CallMethod(NULL, &db_cntl, NULL, NULL, NULL);
        if (db_cntl.Failed()) {
            LOG(ERROR) << __FUNCTION__ << ": " << db_cntl.ErrorText();
            cntl->http_response().set_status_code(503);
            cntl->response_attachment().append(db_cntl.ErrorText());
            return -1;
        }

        LOG(INFO) << db_cntl.response_attachment().to_string();
        cntl->response_attachment() = db_cntl.response_attachment();

        return 0;
    }

    int http_get_dump(
        brpc::Controller* cntl) {
        LOG(INFO) << __FUNCTION__ << ": ================== ";

        brpc::Controller db_cntl;
        /* Request URL */
        db_cntl.http_request().uri() = FLAGS_audio_db_uri + "dump";
        /* POST instead of GET */
        db_cntl.http_request().set_method(brpc::HTTP_METHOD_POST);

        LOG(INFO) << __FUNCTION__ << ": FLAGS_audio_db_uri = " << FLAGS_audio_db_uri;
        LOG(INFO) << __FUNCTION__ << ": db_cntl.http_request().uri() = " << db_cntl.http_request().uri();

        LOG(INFO) << __FUNCTION__ << ": channel_.CallMethod()";
        channel_.CallMethod(NULL, &db_cntl, NULL, NULL, NULL);
        if (db_cntl.Failed()) {
            LOG(ERROR) << __FUNCTION__ << ": " << db_cntl.ErrorText();
            cntl->http_response().set_status_code(503);
            cntl->response_attachment().append(db_cntl.ErrorText());
            return -1;
        }

        LOG(INFO) << __FUNCTION__ << ": " << db_cntl.response_attachment().to_string();
        cntl->response_attachment() = db_cntl.response_attachment();

        return 0;
    }

private:
    std::string encode_hash(const std::unordered_map <int, std::set<int>>& hash) {

        json j_map_4(hash);

        return j_map_4.dump();
    }
    std::unordered_map <int, std::set<int>> decode_hash(std::string hash_str) {

        json shash_json = json::parse(hash_str);

        return shash_json.get<std::unordered_map <int, std::set<int>>>();
    }

    brpc::Channel channel_;
};
