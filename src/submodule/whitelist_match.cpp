#include "submodule/whitelist_match.h"

using namespace bigoai;


WhitelistMatchSubModule::WhitelistMatchSubModule() {}

bool WhitelistMatchSubModule::init(const SubModuleConfig &conf) {
    if (conf.extra_data_size() != 1 || conf.extra_data(0).key() != "result_key") {
        return false;
    }
    result_key_ = conf.extra_data(0).val();
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "imo_discover_url is empty!";
        return false;
    }
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    max_retry_ = conf.rpc().max_retry();
    url_ = conf.rpc().url();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool WhitelistMatchSubModule::call_service(ContextPtr &ctx) {
    auto obj = get_context_data_object(ctx);
    auto detail = obj->detailmlresult();
    // auto flag = false;

    this->to_process_ = false;
    if (detail.find("detection_score") != detail.end() && detail["detection_score"] == "review") {
        this->to_process_ = true;
        // return true;
    } else if (detail.find("porn") != detail.end() && detail["porn"] == "review") {
        this->to_process_ = true;
        // return true;
    } else if (detail.find("imo_pic_Comp") != detail.end() && detail["imo_pic_Comp"] == "review") {
        this->to_process_ = true;
    } else if (detail.find("ocr") != detail.end() && detail["ocr"] == "review") {
        return true;
    }

    if (!this->to_process_)
        return true;

    info_.Clear();
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    Request req;
    req.set_id(std::to_string(get_time_us())); // id 必须可转成 int64, 同一uid下唯一
    req.set_op("query_write");
    req.set_url(get_context_url(ctx));
    req.add_images(ctx->raw_images_size() > 0 ? ctx->raw_images(0) : "");
    req.mutable_videoinfo()->set_country(ctx->normalization_msg().country());
    req.mutable_videoinfo()->set_create_time(std::to_string(ctx->normalization_msg().reporttime()));

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

    if (success_) {
        json2pb::JsonToProtoMessage(result_string, &info_);
    } else {
        add_err_num(1);
    }
    return success_;
}

bool WhitelistMatchSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    batch_info_.clear();
    for (auto &c : ctx) {
        call_service(c);
        batch_info_.emplace_back(info_);
    }
    return true;
}

bool WhitelistMatchSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    CHECK_EQ(ctxs.size(), batch_info_.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i], batch_info_[i]);
    }
    return true;
}

bool WhitelistMatchSubModule::post_process(ContextPtr &ctx) { return post_process(ctx, info_); }

bool WhitelistMatchSubModule::post_process(ContextPtr &ctx, const WhitelistMatchResult &info) {
    if (!this->to_process_) {
        return true;
    }

    if (!success_ || context_is_download_failed(ctx)) {
        set_context_detailmlresult_modelinfo(ctx, "photo_whitelist", MODEL_RESULT_FAIL, "");
    } else if (info.result().empty() || info.result() == info.id() || info.video_status() == 1) {
        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string info_str;
        json2pb::ProtoMessageToJson(info, &info_str, options);
        set_context_detailmlresult_modelinfo(ctx, "photo_whitelist", MODEL_RESULT_PASS, info_str);
    } else {
        add_hit_num(1);

        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string info_str;
        json2pb::ProtoMessageToJson(info, &info_str, options);
        set_context_detailmlresult_modelinfo(ctx, "photo_whitelist", MODEL_RESULT_REVIEW, info_str);
    }
    return true;
}

RegisterClass(SubModule, WhitelistMatchSubModule);
