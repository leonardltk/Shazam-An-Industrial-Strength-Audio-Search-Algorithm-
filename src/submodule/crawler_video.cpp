#include "crawler_video.h"

using namespace bigoai;

RegisterClass(SubModule, CrawlerVideoSubModule);

bool CrawlerVideoSubModule::init(const SubModuleConfig &conf) {
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
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool CrawlerVideoSubModule::call_service(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().video_size() == 0 || ctx->raw_video().size() == 0) {
        return true;
    }

    resp_.Clear();
    Request req;
    req.set_video(ctx->raw_video());
    req.set_image(ctx->raw_cover());
    req.set_id(ctx->traceid());
    for (auto model : models_) {
        req.add_run_model_names(model);
    }
    if (ctx->normalization_msg().appid() != "102055" && country_list_.count(ctx->normalization_msg().country()) != 0) {
        req.add_run_model_names("general_vlog_Race");
    }

    auto url = url_;
    auto channel = channel_manager_.get_channel(url);
    if (channel == nullptr) {
        LOG(ERROR) << "Faield to init channel: " << url;
        return false;
    }
    brpc::Controller cntl;
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, &resp_, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call model service, " << cntl.ErrorText() << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment();
        return false;
    }
    return true;
}

bool CrawlerVideoSubModule::post_process(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().video_size() == 0 || ctx->raw_video().size() == 0) {
        return true;
    }

    std::string country = ctx->normalization_msg().country();
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    for (auto it : resp_.results()) {
        auto model_name = it.first;
        auto model_res = it.second;
        CHECK_GE(model_res.score_size(), 1);
        auto score = model_res.score(0);
        auto threshold = threshold_manager_.get_threshold(country, model_name);
        if (model_name == "like_vlog_Copy") {
            if (ctx->normalization_msg().appid() == "102055") {
                threshold = 1.5;
            } else {
                threshold = 0.61;
            }
        }
        (*video->mutable_threshold())[model_name] = threshold;
        if (score > threshold) {
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
