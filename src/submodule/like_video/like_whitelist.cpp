#include "submodule/like_video/like_whitelist.h"

using namespace bigoai;

bool LikeWhiteListSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}

bool LikeWhiteListSubModule::call_service(ContextPtr& ctx) {
    skip_ = true;
    resp_.Clear();

    // Filter ogic
    if (ctx->video_src() != 4) {
        VLOG(10) << "Not from photo uploading and won't be forwarded to whitelist,"
                 << " source: " << ctx->video_src() << ". [ID:" << ctx->traceid() << "]";
        return true;
    }
    if (ctx->cutme_id() > 0 && ctx->cutme_url().size() != 0) {
        VLOG(10) << "cutme caught. cutmeid: " << ctx->cutme_id() << ", cutmeurl: " << ctx->cutme_url()
                 << " [ID:" << ctx->traceid() << "]";
        return true;
    }
    std::string result_code = (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"];
    if ("4" != result_code) {
        LOG(INFO) << "riskyless data and won't be forwarded to whitelist "
                  << "resultcode: " << result_code << " [ID:" << ctx->traceid() << "]";
        return true;
    }
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    WatermarkWrap::WatermarkInfo watermark;
    std::string watermark_str = (*video->mutable_modelinfo())["copy"];
    json2pb::JsonToProtoMessage(watermark_str, &watermark);
    if (MODEL_RESULT_REVIEW == (*video->mutable_detailmlresult())["copy"] ||
        (watermark.max_retrieval_scores().size() >= 5 && watermark.max_retrieval_scores(4) > 0.9)) {
        LOG(INFO) << "Caught by watermark model and won't be forwarded to whitelist.[ID:" << ctx->traceid() << "]";
        return true;
    }
    if (MODEL_RESULT_REVIEW == (*video->mutable_detailmlresult())["copy"]) {
        LOG(INFO) << "Caught by ann model and won't be forwarded to whitelist [ID:" << ctx->traceid() << "]";
        return true;
    }

    Request req;
    req.set_video(ctx->raw_video());
    req.set_post_id(ctx->traceid());
    req.set_op("query_write");
    VideoMessage videoinfo;
    if (ctx->cutme_id() > 0) {
        videoinfo.set_cutme_id(std::to_string(ctx->cutme_id()));
    }
    videoinfo.set_country(ctx->normalization_msg().country());
    req.set_url(ctx->normalization_msg().data().video(0).content());
    *req.mutable_videoinfo() = {videoinfo};
    skip_ = false;
    GeneralFeatureWrap resp;
    auto ret = call_http_service(ctx, channel_manager_, url_, req, resp);
    if (ret == false) {
        add_err_num();
        return false;
    }
    resp_ = resp.results().begin()->second;

    return true;
}

// enable rewrite.
bool LikeWhiteListSubModule::post_process(ContextPtr& ctx) {
    if (skip_)
        return true;
    if (resp_.result().size() == 0)
        return true;
    const std::string model_name = "whiteList";

    std::string ret_str;
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())[model_name] = ret_str;
    if (resp_.group_id().size() == 0) {
        return true;
    }

    (*ctx->mutable_model_score())[model_name] = std::to_string(resp_.video_status());
    if (resp_.video_status() == 0) {
        add_hit_num();
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "3";
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult())[model_name] =
            MODEL_RESULT_PASS;
        (*ctx->mutable_model_score())[model_name] = "3";
    } else if (resp_.video_status() == 2) {
        (*ctx->mutable_model_score())[model_name] = "4";
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult())[model_name] =
            MODEL_RESULT_REVIEW;
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
    }
    return true;
}


RegisterClass(SubModule, LikeWhiteListSubModule);
