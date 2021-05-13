#include <map>
#include <memory>
#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include <gflags/gflags.h>
#include <brpc/channel.h>
#include "utils/common_utils.h"
#include "live_adaptor.pb.h"
#include "submodule/like_video/like_porn.h"

using namespace bigoai;


bool LikePornSubModule::init(const SubModuleConfig &conf) {
    set_name(conf.instance_name());

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    return true;
}

bool LikePornSubModule::call_service(ContextPtr &ctx) {
    resp_.Clear();
    enable_models_ = {"porn"};
    Request req;

    if (ctx->raw_video().size() == 0) {
        return false;
    }

    req.set_video(ctx->raw_video());
    if (ctx->raw_images_size() != 0 && ctx->raw_images().size() != 0) {
        req.set_cover(ctx->raw_images(0));
    }

    req.set_post_id(ctx->traceid());
    req.set_url("kafka-consumer-cpp");
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    req.set_video_url(ctx->normalization_msg().data().video(0).content());

    // Set attachinfo
    std::string attachinfo_str;
    auto it = ctx->normalization_msg().extradata().find("attachInfo");
    if (it != ctx->normalization_msg().extradata().end()) {
        attachinfo_str = it->second;
    }

    if (ctx->cutme_id() != 0 && ctx->cutme_url().size() != 0) {
        req.set_cutmeid(ctx->cutme_id());
        req.set_cutmeurl(ctx->cutme_url());
    }

    req.set_attachinfo(attachinfo_str);

    auto country = ctx->normalization_msg().country();
    req.add_run_model_names("porn");
    req.add_run_model_thresholds(threshold_manager_.get_threshold(country, "porn"));

    if (ctx->normalization_msg().country() == "IN") {
        enable_models_.push_back("softporn");
        req.add_run_model_names("softporn");
        req.add_run_model_thresholds(0.937);
    }

    // 4.0 Call service
    if (false == call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_)) {
        add_err_num();
        return false;
    }
    return true;
}

// enable rewrite.
bool LikePornSubModule::post_process(ContextPtr &ctx) {
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);

    for (auto m : enable_models_) {
        (*video->mutable_detailmlresult())[m] = MODEL_RESULT_FAIL;
    }

    bool hit_flag = false;
    for (auto model : resp_.results()) {
        auto model_name = model.model_name();
        auto threshold = threshold_manager_.get_threshold(ctx->normalization_msg().country(), model_name);
        if (model.raw_score_size()) {
            auto score = model.raw_score(0);
            (*ctx->mutable_model_score())[model_name] = std::to_string(score);
            if (score > threshold) {
                (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
                (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
                hit_flag = true;
            } else {
                (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
            }
            (*video->mutable_threshold())[model_name] = threshold;
        }
        std::string ret_str;
        json2pb::ProtoMessageToJson(model, &ret_str);
        (*video->mutable_modelinfo())[model_name] = ret_str;
    }
    if (hit_flag) {
        add_hit_num();
    }
    return true;
}

RegisterClass(SubModule, LikePornSubModule);