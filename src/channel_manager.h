#pragma once

#include <brpc/channel.h>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace bigoai {

class ChannelManager {
public:
    std::shared_ptr<brpc::Channel> get_channel(std::string url, const std::string instance_name = "default");
    void set_timeout_ms(int timeout);
    void set_lb(std::string lb);
    void set_retry_time(int retry_time);

private:
    static std::map<std::string, std::shared_ptr<brpc::Channel>> channel_buffer_;
    static std::mutex mutex_;

    bool init_channel(brpc::Channel *channel, const std::string &url);
    std::string lb_ = "rr";
    int timeout_ms_ = 3000;
    int retry_time_ = 0;
};

}
