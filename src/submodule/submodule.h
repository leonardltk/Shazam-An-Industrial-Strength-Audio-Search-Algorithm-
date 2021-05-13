#pragma once

#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <map>
#include <memory>

#include "channel_manager.h"
#include "module/module.h"
#include "module.pb.h"
#include "proxy.pb.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "submodule.pb.h"
#include "submodule/threshold_manager.h"
#include "util.pb.h"
#include "utils/sidecar_proxy.h"
#include "config.pb.h"

DECLARE_bool(log_response_body);

using namespace config;
namespace bigoai {

enum MessageType { IMAGE, VIDEO, AUDIO, TEXT, UNKONWN };

inline MessageType get_message_type(const NormaliztionAuditMessage &msg) {
    if (msg.data().pic_size() != 0)
        return IMAGE;
    if (msg.data().video_size() != 0)
        return VIDEO;
    if (msg.data().voice_size() != 0)
        return AUDIO;
    if (msg.data().text_size() != 0)
        return TEXT;
    return UNKONWN;
}


template<class REQUEST, class RESPONSE>
bool call_http_service(const ContextPtr &ctx, ChannelManager channel_manager, const std::string &url,
                       const REQUEST &req, RESPONSE &resp, int max_retry = 0, bool print_primitive = false) {
    auto channel = channel_manager.get_channel(url);
    if (channel == nullptr) {
        LOG(FATAL) << "Failed to get channel. url: " << url << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    int retry = 0;
    bool flag = false;
    while (retry++ <= max_retry) {
        brpc::Controller cntl;
        cntl.set_always_print_primitive_fields(print_primitive);
        cntl.http_request().uri() = url;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, &resp, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Failed to call " << url << ". " << cntl.ErrorText()
                       << ", status code: " << cntl.http_response().status_code()
                       << ", remote ip: " << cntl.remote_side() << " [ID:" << ctx->traceid() << "]";
            continue;
        }
        flag = true;
        break;
    }
    return flag;
}
template<class REQUEST>
bool call_http_service(const ContextPtr &ctx, ChannelManager channel_manager, const std::string &url,
                       const REQUEST &req, std::string &resp, int max_retry = 0) {
    auto channel = channel_manager.get_channel(url);
    if (channel == nullptr) {
        LOG(FATAL) << "Failed to get channel. url: " << url << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    int retry = 0;
    bool flag = false;
    while (retry++ <= max_retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, NULL, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Failed to call " << url << ". " << cntl.ErrorText() << cntl.http_response().status_code()
                       << " [ID:" << ctx->traceid() << "]";
            continue;
        }
        resp = cntl.response_attachment().to_string();
        flag = true;
        break;
    }
    return flag;
}

class SubModule {
public:
    virtual ~SubModule() {}
    virtual std::string name() const { return name_; }
    virtual void set_name(const std::string &instance_name) { name_ = instance_name; }
    virtual bool call_service(ContextPtr &ctx) { return false; }
    virtual bool post_process(ContextPtr &ctx) { return false; }
    virtual bool init(const SubModuleConfig &conf) {
        set_name(conf.instance_name());
        VLOG(10) << "Init submodule: " << name();
        return true;
    }
    virtual bool call_service_batch(std::vector<ContextPtr> &ctx) {
        LOG(FATAL) << "Not implemented.";
        throw "Not implemented";
    }
    virtual bool post_process_batch(std::vector<ContextPtr> &ctx) {
        LOG(FATAL) << "Not implemented.";
        throw "Not implemented";
    }
    virtual bool is_allow_type(MessageType type) { return true; }

    // public metric
    int get_hit_num() const { return hit_count_; }
    int get_err_num() const { return err_count_; }

    void add_hit_num(int num = 1) { hit_count_ += num; }
    void add_err_num(int num = 1) { err_count_ += num; }
    void clear_metric() {
        hit_count_ = 0;
        err_count_ = 0;
    }

    // public logger
    inline void log_error(const std::string &err_msg, const std::string &id = "") {
        LOG(ERROR) << name() << " id: " << id << " err: " << err_msg;
    }
    inline void log_request_failed(const brpc::Controller &cntl, const std::string &id = "", int32_t retry = 0) {
        LOG(ERROR) << name() << " id: " << id << " retry: " << retry
                   << " status: " << cntl.http_response().status_code() << " ip: " << cntl.remote_side()
                   << " body: " << cntl.response_attachment() << " code: " << cntl.ErrorCode()
                   << " err: " << cntl.ErrorText();
    }
    inline void log_response_body(const brpc::Controller &cntl, const std::string &id = "") {
        if (FLAGS_log_response_body) {
            LOG(INFO) << name() << " id: " << id << " status: " << cntl.http_response().status_code()
                      << " ip: " << cntl.remote_side() << " resp: " << cntl.response_attachment();
        }
    }


protected:
    std::string name_ = "Submodule";

private:
    int err_count_ = 0;
    int hit_count_ = 0;
};
}
