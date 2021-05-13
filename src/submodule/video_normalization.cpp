#include "video_normalization.h"

using namespace bigoai;

RegisterClass(SubModule, VideoNormalizationSubModule);

bool VideoNormalizationSubModule::init(const SubModuleConfig &conf) {
    url_ = conf.rpc().url();
    auto lb = conf.rpc().lb();
    auto retry_time = conf.rpc().max_retry();
    auto timeout_ms = conf.rpc().timeout_ms();

    channel_manager_.set_timeout_ms(timeout_ms);
    channel_manager_.set_retry_time(retry_time);
    channel_manager_.set_lb(lb);
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    models_.insert(models_.end(), conf.enable_model().begin(), conf.enable_model().end());
    CHECK_GE(models_.size(), 1u);
    return true;
}

bool VideoNormalizationSubModule::call_service_batch(std::vector<ContextPtr> &ctxs) {
    resps_.resize(ctxs.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        call_service(ctxs[i], resps_[i]);
    }
    return true;
}

bool VideoNormalizationSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    CHECK_EQ(resps_.size(), ctxs.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i], resps_[i]);
    }
    return true;
}

bool VideoNormalizationSubModule::call_service(ContextPtr &ctx) {
    resps_.resize(1);
    return call_service(ctx, resps_[0]);
}

bool VideoNormalizationSubModule::call_service(ContextPtr &ctx, VideoResponse &resp) {
    if (ctx->normalization_msg().data().video_size() == 0 || ctx->raw_video().size() == 0) {
        return true;
    }

    resp.Clear();
    Request req;
    req.set_video(ctx->raw_video());
    req.set_image(ctx->raw_cover());
    req.set_id(ctx->traceid());
    auto url = url_;
    auto channel = channel_manager_.get_channel(url);
    if (channel == nullptr) {
        LOG(ERROR) << "Faield to init channel: " << url;
        return false;
    }
    brpc::Controller cntl;
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, &resp, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call model service, " << cntl.ErrorText() << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment();
        return false;
    }
    return true;
}

bool VideoNormalizationSubModule::post_process(ContextPtr &ctx) { return post_process(ctx, resps_.at(0)); }

bool VideoNormalizationSubModule::post_process(ContextPtr &ctx, VideoResponse &resp) {
    if (ctx->normalization_msg().data().video_size() == 0 || ctx->raw_video().size() == 0) {
        return true;
    }

    std::string debug;
    json2pb::ProtoMessageToJson(resp, &debug);
    LOG(INFO) << "NormalizationVideo response: " << debug;
    if ((int)models_.size() != resp.results_size()) {
        std::string debug;
        json2pb::ProtoMessageToJson(resp, &debug);
        LOG(INFO) << "Failed to process response. The number of models not equal to " << models_.size()
                  << ", resp: " << debug;
        CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
        for (auto m : models_) {
            auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
            (*video->mutable_detailmlresult())[m] = MODEL_RESULT_FAIL;
        }
        add_err_num(1);
        return false;
    }

    std::string country = ctx->normalization_msg().country();
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    for (auto it : resp.results()) {
        auto model_name = it.first;
        auto model_res = it.second;
        CHECK_GE(model_res.score_size(), 1);
        auto score = model_res.score(0);
        (*video->mutable_threshold())[model_name] = threshold_manager_.get_threshold(country, model_name);
        if (score > threshold_manager_.get_threshold(country, model_name)) {
            (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
            add_hit_num(1);
        } else {
            (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
        }
        std::string debug;
        json2pb::ProtoMessageToJson(model_res, &debug);
        (*video->mutable_modelinfo())[model_name] = debug;
    }
    return true;
}
