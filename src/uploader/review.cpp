#include "review.h"
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>
#include "utils/common_utils.h"

using namespace bigoai;

RegisterClass(Module, ReviewModule);

ChannelManager ReviewModule::channel_manager_;

bool ReviewModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    if (!conf.has_review()) {
        return false;
    }
    work_thread_num_ = conf.work_thread_num();
    channel_manager_.set_timeout_ms(conf.review().timeout_ms());
    channel_manager_.set_lb(conf.review().lb());
    max_retry_ = conf.review().max_retry();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ReviewModule::do_context(ContextPtr &ctx) {
    auto res = call_service(ctx);
    if (res == false) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["review"] = "fail";
    }
    (*ctx->mutable_normalization_msg()->mutable_timestamp())["after_callback"] = get_time_ms();
    (*ctx->mutable_normalization_msg()->mutable_extradata())["ml_review_callback"] = std::to_string(res);
    return res;
}

void check_model_info(AuditDataSubType *data) {
    for (auto it = data->mutable_modelinfo()->begin(); it != data->mutable_modelinfo()->end(); ++it) {
        auto key = it->first;
        auto val = it->second;
        if (val.size() > 65535) {
            LOG(ERROR) << "Ignore " << key << ", value: " << val << ", limit size: 65535.";
            it->second = "{\"msg\":\"value size more than 65535 char\"}";
        }
    }
}

bool ReviewModule::call_service(ContextPtr &ctx) {
    for (int i = 0; i < ctx->normalization_msg().data().video_size(); ++i) {
        auto data = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(i);
        check_model_info(data);
    }
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        auto data = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
        check_model_info(data);
    }
    for (int i = 0; i < ctx->normalization_msg().data().voice_size(); ++i) {
        auto data = ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(i);
        check_model_info(data);
    }

    std::string debug;
    json2pb::ProtoMessageToJson(ctx->normalization_msg(), &debug);
    VLOG(10) << "ReviewCallback: " << debug << "[ID:" << ctx->traceid() << "]";

    std::string callback_url = ctx->normalization_msg().callbackurl();
    auto channel = channel_manager_.get_channel(callback_url);
    if (channel == nullptr) {
        LOG(ERROR) << "Faeild to init channel. url: " << callback_url << "[ID:" << ctx->traceid() << "]";
        return false;
    }

    bool flag = false;
    for (int i = 0; i <= max_retry_; ++i) {
        brpc::Controller cntl;
        cntl.http_request().SetHeader("traceid", ctx->traceid());
        cntl.http_request().uri() = callback_url;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &ctx->normalization_msg(), NULL, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Fail to call review service, " << cntl.ErrorText().substr(0, 500) << "[" << name() << "]"
                       << ", code: " << cntl.ErrorCode() << ", remote_side: " << cntl.remote_side()
                       << ", retried time: " << i << "[" << ctx->traceid() << "]";
            continue;
        }
        VLOG(10) << "Review response: " << cntl.response_attachment();
        flag = true;
        break;
    }

    return flag;
}
