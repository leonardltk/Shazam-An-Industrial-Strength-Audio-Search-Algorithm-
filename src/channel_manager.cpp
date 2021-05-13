#include "channel_manager.h"
#include <brpc/channel.h>
#include <brpc/retry_policy.h>
#include <bthread/bthread.h>
#include <map>
#include <sstream>
#include <thread>

DEFINE_bool(retry_server_error, false, "");

class BigoAIRetryPolicy : public brpc::RetryPolicy {
public:
    // virtual ~BigoAIRetryPolicy() {}

    virtual bool DoRetry(const brpc::Controller *cntl) const override {
        bool flag;
        if (cntl->http_response().status_code() == 302) { // brpc not suport 302 redirect
            auto raw_url = *cntl->http_response().GetHeader("Location");
            LOG(INFO) << "302 redirect " << cntl->http_request().uri() << " => " << raw_url;
            flag = false;
        } else if (FLAGS_retry_server_error && cntl->ErrorCode() == brpc::EHTTP &&
                   cntl->http_response().status_code() >= 500) {
            bthread_usleep((cntl->retried_count()) * 500000); // 500ms
            flag = true;
        } else {
            flag = brpc::DefaultRetryPolicy()->DoRetry(cntl);
        }
        LOG_IF(INFO, cntl->http_response().status_code() != 200)
            << "Failed to access url: " << cntl->http_request().uri() << ". " << cntl->ErrorText()
            << ", status code: " << cntl->http_response().status_code() << ". "
            << "remote side: " << cntl->remote_side() << ". "
            << "retried time: " << cntl->retried_count() << ". "
            << "timeout: " << cntl->timeout_ms() << "ms. "
            << "retry flag: " << flag << ". "
            << "latency: " << cntl->latency_us() / 1000 << "ms. "
            << "[ID:" << cntl->http_request().GetHeader("traceid") << "]";
        return flag;
    }
};
static BigoAIRetryPolicy g_bigo_ai_retry_policy;


namespace bigoai {

std::map<std::string, std::shared_ptr<brpc::Channel>> ChannelManager::channel_buffer_;
std::mutex ChannelManager::mutex_;

bool ChannelManager::init_channel(brpc::Channel *channel, const std::string &url) {
    brpc::ChannelOptions options;
    options.protocol = "http";
    options.timeout_ms = timeout_ms_;
    options.connect_timeout_ms = timeout_ms_ / 2;
    options.retry_policy = &g_bigo_ai_retry_policy;
    options.enable_circuit_breaker = true;
    options.max_retry = retry_time_;
    options.connect_timeout_ms = timeout_ms_ / 2;

    int errcode;
    LOG(INFO) << "Init global channel, url:" << url << ", lb:" << lb_;
    if ((errcode = channel->Init(url.c_str(), lb_.c_str(), &options)) != 0) {
        LOG(INFO) << "Failed to init channel. url: " << url << ", lb: " << lb_;
        return false;
    }
    return true;
}

void ChannelManager::set_timeout_ms(int timeout) { timeout_ms_ = timeout; }

void ChannelManager::set_lb(std::string lb) { lb_ = lb; }

void ChannelManager::set_retry_time(int retry_time) { retry_time_ = retry_time; }

std::shared_ptr<brpc::Channel> ChannelManager::get_channel(std::string url, const std::string instance_name) {
    std::string schema;
    std::string host;
    int port = 0;

    if (0 != brpc::ParseURL(url.c_str(), &schema, &host, &port)) {
        LOG(INFO) << "Failed to parse url: " << url;
        std::shared_ptr<brpc::Channel> ret(new brpc::Channel());
        if (false == init_channel(ret.get(), url)) {
            LOG(INFO) << "Failed to init channel.";
            return nullptr;
        }
        return ret;
    }
    std::string key = url + "/" + instance_name;
    if (port <= 0) {
        std::ostringstream ss;
        if (schema == "http")
            port = 80;
        if (schema == "https")
            port = 443;
        ss << schema << "://" << host << ":" << port << "/" << instance_name;
        key = ss.str();
    } else {
        VLOG(20) << "Port: " << port << ", url: " << url;
    }
    LOG_EVERY_N(INFO, 5000) << "Channel buffer size: " << channel_buffer_.size();
    std::lock_guard<std::mutex> lock(mutex_);
    if (channel_buffer_.find(key) == channel_buffer_.end()) {
        std::shared_ptr<brpc::Channel> ret(new brpc::Channel());
        // LOG(INFO) << "Init endpoint: " << key;
        if (false == init_channel(ret.get(), key)) {
            LOG(INFO) << "Failed to init channel. url: " << url;
            return nullptr;
        }
        channel_buffer_[key] = ret;
    }
    return channel_buffer_[key];
}

} // namespace bigoai
