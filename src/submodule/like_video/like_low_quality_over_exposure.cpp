#include "submodule/like_video/like_low_quality_over_exposure.h"

using namespace bigoai;

bool LikeLowQualityOverExposureSubModule::init(const SubModuleConfig& conf) {
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


bool LikeLowQualityOverExposureSubModule::call_service(ContextPtr& ctx) {
    pb_video_.Clear();

    // 1.0 Set request.
    // video
    Request req_video;
    brpc::Controller cntl;

    req_video.set_url(ctx->normalization_msg().data().video(0).content());
    req_video.set_post_id(ctx->traceid());
    req_video.set_isimage(false);
    req_video.set_op("query");

    res_ = call_http_service(ctx, channel_manager_, url_, req_video, pb_video_, max_retry_);

    if (res_ == false) {
        add_err_num();
    }
    return res_;
}

// enable rewrite.
bool LikeLowQualityOverExposureSubModule::post_process(ContextPtr& ctx) {
    std::string ret_str;
    resp_.Clear();

    if (resp_.mutable_results()->find(enable_model_) == resp_.mutable_results()->end()) {
        LOG(ERROR) << "Key error: " << enable_model_ << " postid: " << ctx->traceid();
        return false;
    }
    const ExposureResponse video_resp_ = pb_video_.results().at(enable_model_);
    ExposureResponse* total_resp_ = &(resp_.mutable_results()->at(enable_model_));

    *total_resp_ = video_resp_;
    total_resp_->add_content_qscores(video_resp_.top3_avg_score());
    total_resp_->add_content_qscores(-1);
    total_resp_->add_content_qscores(-1);
    total_resp_->add_content_qscores(-1);


    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())[enable_model_] = ret_str;
    ret_str.clear();
    return true;
}

RegisterClass(SubModule, LikeLowQualityOverExposureSubModule);