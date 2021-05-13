#include "submodule/like_video/like_low_quality_age.h"

using namespace bigoai;

bool LikeLowQualityAgeSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());
    if (conf.enable_model_size() != 1) {
        LOG(ERROR) << "Failed to parse enable_model field.";
        return false;
    }
    enable_model_ = conf.enable_model(0);

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}


bool LikeLowQualityAgeSubModule::call_service(ContextPtr& ctx) {
    skip_ = true;
    resp_.Clear();
    bool minors_39_flag;
    std::string country = ctx->normalization_msg().country();
    if (country_0313_.count(country)) {
        minors_39_flag = false;
    } else if (country_0309_.count(country)) {
        minors_39_flag = true;
    } else {
        VLOG(10) << "Only execute on specific countries; pass"
                 << " [ID:" << ctx->traceid() << "]"
                 << ", [country:" << ctx->normalization_msg().country() << "]";
        res_ = false;
        return true;
    }

    Request req;
    req.set_op("query");
    req.set_post_id(ctx->traceid());
    req.set_url(ctx->normalization_msg().data().video(0).content());
    req.set_minors_39_flag(minors_39_flag);

    skip_ = false;
    res_ = call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_);
    if (res_ == false) {
        add_err_num();
    }
    return res_;
}

// enable rewrite.
bool LikeLowQualityAgeSubModule::post_process(ContextPtr& ctx) {
    if (skip_)
        return true;
    std::string ret_str;
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())[enable_model_] = ret_str;
    (*ctx->mutable_model_score())[enable_model_] = resp_.result();
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult())[enable_model_] =
        MODEL_RESULT_PASS;
    if (resp_.result() == "1") {
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult())[enable_model_] =
            MODEL_RESULT_REVIEW;
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        add_hit_num();
    }
    return true;
}

RegisterClass(SubModule, LikeLowQualityAgeSubModule);