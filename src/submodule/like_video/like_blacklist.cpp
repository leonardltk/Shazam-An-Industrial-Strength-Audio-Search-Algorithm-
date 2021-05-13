#include "submodule/like_video/like_blacklist.h"

using namespace bigoai;

bool LikeBlackListSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}

bool LikeBlackListSubModule::call_service(ContextPtr& ctx) {
    skip_ = true;
    resp_.Clear();
    if (ctx->video_src() != 4) {
        VLOG(10) << "Not from photo uploading and won't be forwarded to blacklist. source: " << ctx->video_src()
                 << ". [ID:" << ctx->traceid() << "]";
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

    std::string debug;
    GeneralFeatureWrap resp;
    skip_ = false;
    auto ret = call_http_service(ctx, channel_manager_, url_, req, resp);
    if (resp.results_size() == 1) {
        resp_ = resp.results().begin()->second;
        json2pb::ProtoMessageToJson(resp_, &debug);
        VLOG(10) << "blacklist: " << debug;
    } else {
        add_err_num(1);
    }

    return ret;
}

// enable rewrite.
bool LikeBlackListSubModule::post_process(ContextPtr& ctx) {
    if (skip_)
        return true;
    if (resp_.result().size() == 0)
        return true;


    std::string ret_str;
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())["blacklistauto"] =
        ret_str;

    if (resp_.group_id().size() == 0) {
        return true;
    }


    if (resp_.video_status() == 1) {
        (*ctx->mutable_model_score())["blacklistauto"] = "1";
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*ctx->mutable_normalization_msg()
              ->mutable_data()
              ->mutable_video(0)
              ->mutable_detailmlresult())["blacklistauto"] = MODEL_RESULT_REVIEW;
        add_hit_num(1);
    } else {
        (*ctx->mutable_model_score())["blacklistauto"] = "0";
        (*ctx->mutable_normalization_msg()
              ->mutable_data()
              ->mutable_video(0)
              ->mutable_detailmlresult())["blacklistauto"] = MODEL_RESULT_PASS;
    }
    return true;
}


RegisterClass(SubModule, LikeBlackListSubModule);
