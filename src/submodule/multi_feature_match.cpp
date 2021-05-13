#include "submodule/multi_feature_match.h"


using namespace bigoai;

bool MultiFeatureMatchSubModule::init(const SubModuleConfig &conf) {
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

bool MultiFeatureMatchSubModule::call_service(ContextPtr &ctx) {
    infos_.clear();
    successes_.clear();

    auto channel = channel_manager_.get_channel(url_);
    for (int i = 0; i <  ctx->normalization_msg().data().pic_size(); ++i) {
        bool single_success = false;
        std::string result_string;
        Request req;
        std::string info;
        req.set_id(std::to_string(get_time_us())); // id 必须可转成 int64, 同一uid下唯一
        req.set_op("query");
        req.set_video(ctx->raw_images(i));
        for (int retry = 0; !single_success && retry <= max_retry_; ++retry) {
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
            if (!(parse_proxy_sidecar_response(resp_body, storename_, result_string, status_code) && status_code == 200)) {
                log_error("Failed to parse response or status code is not 200.", ctx->traceid());
                continue;
            }
            single_success = true;
        }

        if (single_success) {
            FeatureMatchResponse resp;
            json2pb::JsonToProtoMessage(result_string, &resp);
            if (req.id() != resp.result()) { // 返回的 result 与请求 id 不一致则命中
                if (!feature_info_query(ctx, resp.result(), info)) {
                    single_success = false;
                }
            }
        } else {
            add_err_num(1);
        }
        successes_.push_back(single_success);
        infos_.push_back(info);
    }
    for (size_t i = 0; i < successes_.size(); ++i) {
        if (!successes_[i]) return false;
    }
    return true;
}

bool MultiFeatureMatchSubModule::post_process(ContextPtr &ctx) { 
    if ((size_t)ctx->normalization_msg().data().pic_size() != successes_.size() 
        || (size_t)ctx->normalization_msg().data().pic_size() != infos_.size()) {
        log_error("Pic size and successes_size or infos_size mismatch!");
            return false;
    }
    const std::string model_name = "feature_match";
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
        (*image->mutable_modelinfo())[model_name] = infos_[i];
        if (!successes_[i]) {
            (*image->mutable_detailmlresult())[model_name] = MODEL_RESULT_FAIL;
        } else {
            if (infos_[i].empty()) {
                (*image->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
            } else {
                add_hit_num(1);
                (*image->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
            }
        }
    }
    return true;
}

bool MultiFeatureMatchSubModule::feature_info_query(ContextPtr &ctx, const std::string &featue_id, std::string& info) {
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
            info = resp_body;
        }
        success = true;
    }
    return success;
}

RegisterClass(SubModule, MultiFeatureMatchSubModule);
