#pragma once
#include <brpc/channel.h>
#include <brpc/policy/redis_authenticator.h>
#include <brpc/redis.h>
#include <gflags/gflags.h>
#include <vector>

namespace bigoai {

class PikaClient {
public:
    PikaClient(int timeout_ms, int passwd_index, int expire_second, std::string hosts, std::string prefix);
    ~PikaClient();
    bool is_exist(const std::string &postid);
    bool write(const std::string &postid);

private:
    int passwd_index_;
    int expire_second_;
    std::string prefix_;
    brpc::Channel channel_;
    brpc::policy::RedisAuthenticator *auth_ = nullptr;
    const std::vector<std::string> passwd_ = {"Common_789#$^", "Indigo_346#$^", "Duowan_123!"};
};

}
