#include "submodule/like_video/like_low_quality_exposure.h"

using namespace bigoai;

bool LikeLowQualityExposureSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());
    if (conf.extra_data_size() != 1 || conf.extra_data(0).key() != "enable_cover") {
        return false;
    }
    enable_cover_ = std::stoi(conf.extra_data(0).val());

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


bool LikeLowQualityExposureSubModule::call_service(ContextPtr& ctx) {
    pb_img_.Clear();
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

    // image
    if (!enable_cover_) {
        return res_;
    }

    Request req_img;
    if (ctx->raw_images_size() != 0 && ctx->raw_images(0).size() != 0)
        req_img.add_images(ctx->raw_images(0));
    else
        req_img.set_url(ctx->normalization_msg().data().pic(0).content());
    req_img.set_post_id(ctx->traceid());
    req_img.set_isimage(true);
    req_img.set_op("query");

    res_ = call_http_service(ctx, channel_manager_, url_, req_img, pb_img_, max_retry_);
    if (res_ == false) {
        add_err_num();
    }
    return res_;
}

// enable rewrite.
bool LikeLowQualityExposureSubModule::post_process(ContextPtr& ctx) {
    std::string ret_str;
    json2pb::ProtoMessageToJson(pb_video_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())[enable_model_] = ret_str;
    ret_str.clear();
    if (!enable_cover_) {
        return true;
    }
    json2pb::ProtoMessageToJson(pb_img_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0)->mutable_modelinfo())[enable_model_] = ret_str;
    return true;
}

RegisterClass(SubModule, LikeLowQualityExposureSubModule);