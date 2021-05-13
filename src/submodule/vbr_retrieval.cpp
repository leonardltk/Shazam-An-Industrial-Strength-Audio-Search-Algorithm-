//#include <random>

#include "vbr_retrieval.h"

using namespace bigoai;

RegisterClass(SubModule, VbrRetrievalSubModule);

bool VbrRetrievalSubModule::init(const SubModuleConfig &conf) {
    if (conf.extra_data_size() != 2 || conf.extra_data(0).key() != "result_key" || conf.extra_data(1).key() != "key") {
        return false;
    }
    result_key_ = conf.extra_data(0).val();
    key_ = conf.extra_data(1).val();

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool VbrRetrievalSubModule::call_service(ContextPtr &ctx) {
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    auto random_num = ctx->normalization_msg().uid().substr(0, 5);
    auto time = std::to_string(get_time_us());
    auto req_id = time;

    VbrRetrievalRequest req;
    req.set_id(req_id); // todo: req_id needs modification
    req.set_op("query_write");
    // todo: fill info for the below
    req.set_url(get_context_url(ctx));
    if (ctx->normalization_msg().data().pic_size() || ctx->normalization_msg().data().live_size()) {
        req.set_video(ctx->raw_images(0));
    } else if (ctx->normalization_msg().data().video_size()) {
        req.set_video(ctx->raw_video());
    }
    req.set_business(ctx->normalization_msg().businesstype());
    req.mutable_videoinfo()->set_country(ctx->normalization_msg().country());
    req.mutable_videoinfo()->set_create_time(std::to_string(ctx->normalization_msg().reporttime()));
    req.mutable_videoinfo()->set_uid(ctx->normalization_msg().uid());

    auto channel = channel_manager_.get_channel(url_);

    success_ = false;
    std::string result_string;
    for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
        // 3.0 Call service
        brpc::Controller cntl;

        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, NULL, NULL);
        if (cntl.Failed()) {
            log_request_failed(cntl, ctx->traceid(), retry);
            continue;
        }
        log_response_body(cntl, ctx->traceid());

        auto resp_body = cntl.response_attachment().to_string();
        int status_code = 0;
        if (!(parse_proxy_sidecar_response(resp_body, result_key_, result_string, status_code) && status_code == 200)) {
            log_error("Failed to parse response or status code is not 200.", ctx->traceid());
            continue;
        }

        success_ = true;
    }

    if (!success_) {
        add_err_num(1);
    }
    return success_;
}

bool VbrRetrievalSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    for (auto &c : ctx) {
        call_service(c);
    }
    return true;
}

bool VbrRetrievalSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i]);
    }
    return true;
}

bool VbrRetrievalSubModule::post_process(ContextPtr &ctx) {
    if (success_ == false || context_is_download_failed(ctx)) {
        set_context_modelinfo(ctx, key_, "fail");
    } else {
        set_context_modelinfo(ctx, key_, "success");
    }
    return true;
}