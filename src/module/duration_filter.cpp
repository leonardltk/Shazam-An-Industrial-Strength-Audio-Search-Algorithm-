#include "module/duration_filter.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <thread>
#include "util.pb.h"
#include "utils/base64.h"
#include "utils/common_utils.h"


namespace bigoai {
bool DurationFilterModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "DurationFilterModule");
    if (!conf.has_duration_filter()) {
        return false;
    }

    work_thread_num_ = conf.work_thread_num();
    max_second_ = conf.duration_filter().max_second();
    min_second_ = conf.duration_filter().min_second();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool DurationFilterModule::do_context(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().video_size() == 1) {
        if (ctx->normalization_msg().data().video(0).subextradata().find("duration") ==
            ctx->normalization_msg().data().video(0).subextradata().end()) {
            LOG(INFO) << "Not found duration field. [ID:" << ctx->traceid() << "]";
        } else if (std::stof(ctx->normalization_msg().data().video(0).subextradata().at("duration")) > max_second_) {
            ctx->mutable_enable_modules()->clear();
            (*ctx->mutable_enable_modules())["UploadHiveModule"] = 1;
            (*ctx->mutable_enable_modules())["ReviewModule"] = 1;
            LOG(INFO) << "Filter message. duration: "
                      << ctx->normalization_msg().data().video(0).subextradata().at("duration")
                      << ", greater than : " << max_second_ << "[ID:" << ctx->traceid() << "]";
        }
    }

    if (ctx->normalization_msg().data().voice_size() == 1) {
        if (ctx->normalization_msg().data().voice(0).subextradata().find("duration") ==
            ctx->normalization_msg().data().voice(0).subextradata().end()) {
            LOG(INFO) << "Not found duration field. [ID:" << ctx->traceid() << "]";
        } else if (std::stof(ctx->normalization_msg().data().voice(0).subextradata().at("duration")) < min_second_) {
            ctx->mutable_enable_modules()->clear();
            (*ctx->mutable_enable_modules())["UploadHiveModule"] = 1;
            LOG(INFO) << "Filter message. duration: "
                      << ctx->normalization_msg().data().voice(0).subextradata().at("duration")
                      << ", less than : " << min_second_ << "[ID:" << ctx->traceid() << "]";
        }
    }
    return true;
}

RegisterClass(Module, DurationFilterModule);
} // namespace bigoai
