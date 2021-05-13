#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/like_video/like_audio_sim.h"

using namespace bigoai;

bool LikeAudioSimSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());

    for (auto country : conf.enable_country()) {
        enable_country_.insert(country);
    }

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}

bool LikeAudioSimSubModule::call_service(ContextPtr& ctx) {
    skip_ = true;
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    if (ctx->raw_video().size() == 0) {
        LOG(INFO) << "Empty data. [ID:" << ctx->traceid() << "]";
        return true;
    }
    if (enable_country_.count(ctx->normalization_msg().country()) == 0) {
        VLOG(10) << "Skip " << name() << ", not in expect country list.";
        return true;
    }

    Request req;
    resp_.Clear();
    req.set_id(ctx->traceid());
    req.set_op("query");
    req.set_video(ctx->raw_video());

    skip_ = false;
    // 3.0 Call service
    auto ret = call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_);
    if (ret == false)
        add_err_num(1);
    return ret;
}

// enable rewrite.
bool LikeAudioSimSubModule::post_process(ContextPtr& ctx) {
    if (skip_)
        return true;
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    std::string ret_str;
    const std::string model_name = "audio_audit";
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*video->mutable_modelinfo())[model_name] = ret_str;
    if (resp_.results_size() != 1)
        return false;
    auto act = resp_.results().begin()->second.act();
    (*ctx->mutable_model_score())[model_name] = std::to_string(act);
    (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
    if (act > 0) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
        add_hit_num(1);
    }
    return true;
}

RegisterClass(SubModule, LikeAudioSimSubModule);
