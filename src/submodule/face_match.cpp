#include "submodule/face_match.h"

// TODO is the same with face_match.cpp

using namespace bigoai;
FaceMatchSubModule::FaceMatchSubModule() {}

bool FaceMatchSubModule::init(const SubModuleConfig &conf) {
    if (!conf.has_feature_match()) {
        return false;
    }

    if (conf.rpc().url().size() == 0 || conf.feature_match().query_rpc().url().size() == 0) {
        LOG(ERROR) << "emtpy url config!";
        return false;
    }

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    query_channel_manager_.set_timeout_ms(conf.feature_match().query_rpc().timeout_ms());
    query_channel_manager_.set_lb(conf.feature_match().query_rpc().lb());
    query_url_ = conf.feature_match().query_rpc().url();
    query_max_retry_ = conf.feature_match().query_rpc().max_retry();
    storename_ = conf.feature_match().storename();
    auto query_channel = channel_manager_.get_channel(query_url_);
    if (query_channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << query_url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool FaceMatchSubModule::call_service(ContextPtr &ctx) {
    info_.clear();
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    Request req;
    req.set_id(std::to_string(get_time_us())); // id 必须可转成 int64, 同一uid下唯一
    req.set_op("query");
    if (ctx->normalization_msg().data().pic_size()) {
        req.set_image(ctx->raw_images_size() > 0 ? ctx->raw_images(0) : "");
    } else {
        LOG(ERROR) << "Call FaceMatchSubModule with no pic.";
        return false;
    }
    req.set_url(get_context_url(ctx));

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

        if (cntl.http_response().status_code() > 200 && cntl.http_response().status_code() < 300) {
            success_ = true;
            return success_; // 无人脸的正常返回
        }

        auto resp_body = cntl.response_attachment().to_string();
        int status_code = 0;
        if (!(parse_proxy_sidecar_response(resp_body, storename_, result_string, status_code) && status_code == 200)) {
            log_error("Failed to parse response or status code is not 200.", ctx->traceid());
            continue;
        }
        success_ = true;
    }

    if (success_) {
        FeatureMatchResponse resp;
        json2pb::JsonToProtoMessage(result_string, &resp);
        if (req.id() != resp.result()) { // 返回的 result 与请求 id 不一致则命中
            if (!feature_info_query(ctx, resp.result())) {
                success_ = false;
            }
        }
    } else {
        add_err_num(1);
    }
    return success_;
}

bool FaceMatchSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    batch_info_.clear();
    for (auto &c : ctx) {
        call_service(c);
        batch_info_.emplace_back(info_);
    }
    return true;
}

bool FaceMatchSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    CHECK_EQ(ctxs.size(), batch_info_.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i], batch_info_[i]);
    }
    return true;
}

bool FaceMatchSubModule::post_process(ContextPtr &ctx) { return post_process(ctx, info_); }

bool FaceMatchSubModule::post_process(ContextPtr &ctx, const std::string &info) {
    if (!success_ || context_is_download_failed(ctx)) {
        set_context_detailmlresult_modelinfo(ctx, "face_match", MODEL_RESULT_FAIL, info);
    } else if (info.empty()) {
        set_context_detailmlresult_modelinfo(ctx, "face_match", MODEL_RESULT_PASS, info);
    } else {
        add_hit_num(1);
        set_context_detailmlresult_modelinfo(ctx, "face_match", MODEL_RESULT_REVIEW, info);
    }
    return true;
}

bool FaceMatchSubModule::feature_info_query(ContextPtr &ctx, const std::string &featue_id) {
    FeatureMatchInfoQueryRequest req;
    req.set_featureid1(featue_id);
    req.set_appid(ctx->normalization_msg().appid());
    req.set_storename(storename_);
    req.set_url(get_context_url(ctx));

    auto channel = channel_manager_.get_channel(query_url_);

    bool success = false;
    for (int retry = 0; !success && retry <= query_max_retry_; ++retry) {
        // 3.0 Call service
        brpc::Controller cntl;

        cntl.http_request().uri() = query_url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);

        channel->CallMethod(NULL, &cntl, &req, NULL, NULL);
        if (cntl.Failed()) {
            log_request_failed(cntl, ctx->traceid(), retry);
            return false;
        }
        log_response_body(cntl, ctx->traceid());

        auto resp_body = cntl.response_attachment().to_string();

        rapidjson::Document json_obj;
        json_obj.Parse(resp_body.c_str());
        if (!(json_obj.HasMember("code") && json_obj["code"].IsInt() && json_obj["code"].GetInt() == 0)) {
            log_error("response code non-zero. resp: " + resp_body, ctx->traceid());
            continue;
        }
        if (!(json_obj.HasMember("data") && json_obj["data"].IsObject())) {
            log_error("feature info not found. resp: " + resp_body, ctx->traceid());
            continue;
        }

        auto data_obj = json_obj["data"].GetObject();
        if (data_obj.HasMember("id") && data_obj["id"].IsString() && data_obj["id"].Size() > 0) {
            info_ = resp_body;
        }
        success = true;
    }
    return success;
}

RegisterClass(SubModule, FaceMatchSubModule);
