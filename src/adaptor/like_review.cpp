#include "adaptor/like_review.h"
#include "utils/common_utils.h"
#include "live_adaptor.pb.h"
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <openssl/md5.h>
using namespace bigoai;


RegisterClass(Module, LikeReviewModule);


std::string MD5(const std::string& src) {
    MD5_CTX ctx;

    std::string md5_string;
    unsigned char md[16] = {0};
    char tmp[33] = {0};

    MD5_Init(&ctx);
    MD5_Update(&ctx, src.c_str(), src.size());
    MD5_Final(md, &ctx);

    for (int i = 0; i < 16; ++i) {
        memset(tmp, 0x00, sizeof(tmp));
        sprintf(tmp, "%02X", md[i]);
        md5_string += tmp;
    }
    return md5_string;
}

bool LikeReviewModule::init(const ModuleConfig& conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    if (!conf.has_review()) {
        return false;
    }

    appid_ = conf.review().appid();
    work_thread_num_ = conf.work_thread_num();
    channel_manager_.set_timeout_ms(conf.downloader().timeout_ms());
    channel_manager_.set_lb(conf.downloader().lb());
    channel_manager_.set_retry_time(0);
    max_retry_ = conf.review().max_retry();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool LikeReviewModule::do_context(ContextPtr& ctx) {
    LikeReviewMessage cb;
    cb.set_uid(ctx->normalization_msg().uid());
    cb.set_videoid(ctx->traceid());
    if (ctx->normalization_msg().data().video_size() != 1) {
        LOG(ERROR) << "Not found video data. id: " << ctx->traceid();
        return false;
    }
    std::istringstream is(
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_subextradata())["duration"]);
    float duration;
    is >> duration;
    duration /= 1000.0;
    cb.set_dura(std::to_string(duration));
    cb.set_source("3");
    cb.set_appid(appid_);
    cb.set_extpar((*ctx->mutable_normalization_msg()->mutable_extradata())["extInfo"]);
    cb.set_videourl(ctx->normalization_msg().data().video(0).content());
    cb.set_traceid(ctx->traceid());
    if ((*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"].size() == 0) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "3";
    }
    cb.set_resultcode((*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"]);

    // Build attachinfo payload.
    LikeReviewMessage::AttachInfo attach;
    attach.set_feature_id1((*ctx->mutable_normalization_msg()->mutable_extradata())["feature_id1"]);
    attach.set_feature_id2((*ctx->mutable_normalization_msg()->mutable_extradata())["feature_id2"]);

    for (auto it : ctx->normalization_msg().data().video(0).detailmlresult()) {
        Map m;
        auto model_name = it.first;
        auto model_res = it.second;
        m.set_key(model_name);
        if (model_res == MODEL_RESULT_PASS) {
            m.set_value("3");
        } else {
            m.set_value("4");
        }

        // TODO: delete the code.
        if (model_name == "general_ann_caught" && model_res == MODEL_RESULT_PASS) {
            m.set_value("");
        }
        if (model_name == "general_kakashi") {
            m.set_value((*ctx->mutable_normalization_msg()->mutable_extradata())["general_kakashi"]);
        }
        *attach.add_modelresults() = m;
    }
    for (auto it : ctx->normalization_msg().data().pic(0).detailmlresult()) {
        Map m;
        auto model_name = it.first;
        auto model_res = it.second;
        m.set_key(model_name);
        if (model_res == MODEL_RESULT_PASS) {
            m.set_value("3");
        } else {
            m.set_value("4");
        }
        *attach.add_modelresults() = m;
    }
    for (auto it : ctx->model_score()) {
        Map m;
        auto model_name = it.first;
        auto model_score = it.second;
        m.set_key(model_name);
        m.set_value(model_score);
        *attach.add_modelscores() = m;
    }

    std::string debug;
    json2pb::ProtoMessageToJson(attach, &debug);
    *cb.mutable_attachinfo() = debug;
    (*ctx->mutable_normalization_msg()->mutable_extradata())["review"] = debug;

    int retry_time = 0;
    bool flag = false;
    while (retry_time++ <= max_retry_) {
        flag = do_upload(cb, ctx);
        if (flag == true) {
            break;
        }
    }
    if (flag == false) {
        LOG(ERROR) << "Failed to call people review service finally. Discard: " << ctx->traceid();
    }
    (*ctx->mutable_normalization_msg()->mutable_extradata())["ml_review_callback"] = std::to_string(flag);
    return flag;
}

bool LikeReviewModule::do_upload(const LikeReviewMessage& item, const ContextPtr& ctx) {
    const std::string secret = "gg9wvXPf9MJzpGfwyGD0fvjuPqpXRoza";

    brpc::Controller cntl;
    auto url = ctx->normalization_msg().callbackurl();
    cntl.http_request().uri() = url;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    cntl.http_request().set_content_type("application/json");

    int64_t timestamp = get_time_ms();
    std::string body_str;
    json2pb::ProtoMessageToJson(item, &body_str);
    LikeReviewRequest req;
    req.set_body(body_str);
    req.set_timestamp(timestamp);
    req.set_sign(MD5(body_str + std::to_string(timestamp) + secret));

    std::string payload;
    json2pb::ProtoMessageToJson(req, &payload);
    LOG(INFO) << "Like review: " << payload;
    cntl.request_attachment().append(payload);
    LikeReviewResponse resp;
    auto channel = channel_manager_.get_channel(url);
    // channel->CallMethod(NULL, &cntl, NULL, &resp, NULL);
    channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);

    if (cntl.Failed() || cntl.http_response().status_code() != 200) {
        LOG(ERROR) << "Failed to call like review interface. ip: " << cntl.remote_side()
                   << ", http status: " << cntl.http_response().status_code() << ", error: " << cntl.ErrorText()
                   << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    if (resp.code() != 0) {
        LOG(FATAL) << "Failed to call like review interface. ip: " << cntl.remote_side()
                   << ", error: " << cntl.ErrorText() << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    return true;
}
