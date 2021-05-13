#include "submodule/imo_group_chat.h"


using namespace bigoai;


ImoGroupChatSubModule::ImoGroupChatSubModule() {}

bool ImoGroupChatSubModule::init(const SubModuleConfig &conf) {
    if (conf.extra_data_size() != 1 || conf.extra_data(0).key() != "result_key") {
        return false;
    }
    result_key_ = conf.extra_data(0).val();
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImoGroupChatSubModule::call_service(ContextPtr &ctx) {
    resp_.Clear();
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    auto req_id = std::to_string(get_time_us());

    ImoGroupChatMatchRequest req;
    req.set_id(req_id); // todo: req_id needs modification
    req.set_op("query_write");
    req.set_url(get_context_url(ctx));
    if (ctx->normalization_msg().data().pic_size() || ctx->normalization_msg().data().live_size()) {
        req.set_video(ctx->raw_images(0));
    } else if (ctx->normalization_msg().data().video_size()) {
        req.set_video(ctx->raw_video());
    }
    req.mutable_videoinfo()->set_country("Global");
    req.mutable_videoinfo()->set_create_time(std::to_string(ctx->normalization_msg().reporttime()));
    req.mutable_videoinfo()->set_info_id(ctx->normalization_msg().orderid());

    auto channel = channel_manager_.get_channel(url_);

    success_ = false;
    std::string result_string;
    for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
        // 3.0 Call service
        brpc::Controller cntl;

        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, NULL, NULL);
        VLOG(10) << "Imo feature response: " << cntl.response_attachment();
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

        json2pb::JsonToProtoMessage(result_string, &resp_);
        success_ = true;
    }

    if (!success_) {
        add_err_num(1);
    }
    return success_;
}

bool ImoGroupChatSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    batch_resp_.clear();
    for (auto &c : ctx) {
        call_service(c);
        batch_resp_.emplace_back(resp_);
    }
    return true;
}

bool ImoGroupChatSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    CHECK_EQ(ctxs.size(), batch_resp_.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i], batch_resp_[i]);
    }
    return true;
}

bool ImoGroupChatSubModule::post_process(ContextPtr &ctx) { return post_process(ctx, resp_); }

bool ImoGroupChatSubModule::post_process(ContextPtr &ctx, const ImoGroupChatMatchResponse &resp) {
    if (success_ == false || context_is_download_failed(ctx)) {
        set_context_detailmlresult_modelinfo(ctx, "group_chat_vega", MODEL_RESULT_FAIL, "");
    } else if (resp.result() == resp.unique_id() || resp.distance() <= 0) {
        set_context_detailmlresult_modelinfo(ctx, "group_chat_vega", MODEL_RESULT_PASS, "");
    } else {
        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string group_chat_vega_str;
        json2pb::ProtoMessageToJson(resp, &group_chat_vega_str, options);

        add_hit_num(1);
        set_context_detailmlresult_modelinfo(ctx, "group_chat_vega", MODEL_RESULT_REVIEW, group_chat_vega_str);
    }
    return true;
}


RegisterClass(SubModule, ImoGroupChatSubModule);
