#include "pika_client.h"
#include <brpc/policy/redis_authenticator.h>
#include <brpc/redis.h>
#include <vector>


// Best try to avoid duplication.
namespace bigoai {

PikaClient::PikaClient(int timeout_ms, int passwd_index, int expire_second, std::string hosts, std::string prefix) {
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_REDIS;
    options.timeout_ms = timeout_ms;
    auth_ = new brpc::policy::RedisAuthenticator(passwd_.at(passwd_index));
    options.auth = auth_;
    expire_second_ = expire_second;
    prefix_ = prefix;
    LOG_IF(FATAL, channel_.Init(hosts.c_str(), "la", &options)) << "Failed to init channel to redis-server";
}

PikaClient::~PikaClient() { delete auth_; }

bool PikaClient::is_exist(const std::string &postid) {
    brpc::Controller cntl;
    brpc::RedisRequest req;
    brpc::RedisResponse resp;

    req.AddCommand("GET %s", (prefix_ + postid.c_str()).c_str());

    channel_.CallMethod(NULL, &cntl, &req, &resp, NULL);

    if (cntl.Failed()) {
        LOG(ERROR) << "Failed to access redis-server."
                   << " ErrText: " << cntl.ErrorText();
        return false;
    }
    if (resp.reply(0).is_error()) {
        LOG(ERROR) << "Failed to query postid: " << postid << ", pika return error.";
        return false;
    }
    if (resp.reply(0).type() == brpc::REDIS_REPLY_NIL) {
        //#LOG(ERROR) << "Not found postid: " << postid << ", pika return nil.";
        return false;
    }
    return true;
}

bool PikaClient::write(const std::string &postid) {
    brpc::Controller cntl;
    brpc::RedisRequest req;
    brpc::RedisResponse resp;

    std::string data = "1";
    req.AddCommand("SET  %s %b EX %d", (prefix_ + postid).c_str(), data.data(), data.size(), expire_second_);
    channel_.CallMethod(NULL, &cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Failed to write redis-server."
                   << " ErrText: " << cntl.ErrorText();
        return false;
    }
    return true;
}

}
