#include "submodule/imo_video_core.h"

using namespace bigoai;

ImoVideoCoreSubModule::ImoVideoCoreSubModule() {}

bool ImoVideoCoreSubModule::init(const SubModuleConfig &conf) {
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "url is empty!";
        return false;
    }

    url_ = conf.rpc().url();
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    max_retry_ = conf.rpc().max_retry();
    auto channel = channel_manager_.get_channel(conf.rpc().url());

    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << conf.rpc().url();
        return false;
    }

    model_names_.insert(model_names_.end(), conf.enable_model().begin(), conf.enable_model().end());

    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImoVideoCoreSubModule::call_service(ContextPtr &ctx) {
    resp_.Clear();
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    Request req;
    req.set_video(ctx->raw_video());
    req.set_post_id(ctx->normalization_msg().orderid());
    req.set_url(get_context_url(ctx));

    auto channel = channel_manager_.get_channel(url_);

    bool success = false;
    for (int retry = 0; !success && retry <= max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(nullptr, &cntl, &req, nullptr, nullptr);
        if (cntl.Failed()) {
            log_request_failed(cntl, ctx->traceid(), retry);
            continue;
        }
        log_response_body(cntl, ctx->traceid());

        json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp_);
        if (resp_.status() != 200) {
            log_error("status is not 200!", ctx->traceid());
            continue;
        }
        success = true;
    }
    return success;
}

bool ImoVideoCoreSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    batch_resp_.clear();
    for (auto &c : ctx) {
        call_service(c);
        batch_resp_.emplace_back(resp_);
    }
    return true;
}

bool ImoVideoCoreSubModule::post_process(ContextPtr &ctx, ImoVideoCoreResponse &resp) {
    if (context_is_download_failed(ctx)) {
        for (auto &model_name : model_names_) {
            set_context_detailmlresult_modelinfo(ctx, model_name, MODEL_RESULT_FAIL, "");
        }
        return true;
    }
    std::string country = ctx->normalization_msg().country();
    for (int i = 0; i < resp_.results_size(); ++i) {
        auto score = resp_.results(i).raw_score_size() > 0 ? resp_.results(i).raw_score(0) : 0.0;

        auto res = MODEL_RESULT_PASS;
        auto model_name = resp_.results(i).model_name();
        float threshold = threshold_manager_.get_threshold(country, model_name);
        if (score > threshold) {
            res = MODEL_RESULT_REVIEW;
        }

        ModelInfo info;
        info.set_score(score);
        info.set_threshold(threshold);

        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string info_str;
        json2pb::ProtoMessageToJson(info, &info_str, options);

        set_context_detailmlresult_modelinfo(ctx, resp_.results(i).model_name(), res, info_str);
    }

    std::string ctx_str;
    json2pb::ProtoMessageToJson(ctx->normalization_msg(), &ctx_str);
    LOG(INFO) << ctx_str;
    return true;
}

bool ImoVideoCoreSubModule::post_process(ContextPtr &ctx) { return post_process(ctx, resp_); }

bool ImoVideoCoreSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    CHECK_EQ(ctxs.size(), batch_resp_.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i], batch_resp_[i]);
    }
    return true;
}


RegisterClass(SubModule, ImoVideoCoreSubModule);
