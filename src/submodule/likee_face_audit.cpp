#include "submodule/likee_face_audit.h"

// TODO is the same with likee_face_audit.cpp

using namespace bigoai;
LikeeFaceAuditSubModule::LikeeFaceAuditSubModule() {}

bool LikeeFaceAuditSubModule::init(const SubModuleConfig &conf) {
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
    for (auto whitelist_uid: conf.feature_match().whitelist_uids()) {
        whitelist_uids_[whitelist_uid] = 1;
    }
    auto query_channel = channel_manager_.get_channel(query_url_);
    if (query_channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << query_url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool LikeeFaceAuditSubModule::call_service(ContextPtr &ctx) {
    info_.clear();
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    // 全量
    // 按国家放量
    // if (!enable_countrys_.count(ctx->normalization_msg().country())) {
    //     // LOG(INFO) << "Only execute on specific countries; pass country: " << ctx->normalization_msg().country() << ".";
    //     return true;
    // }

    FaceAuditRequest req;
    req.set_op("query_write");
    req.set_uid(ctx->normalization_msg().uid());
    req.set_url(get_context_url(ctx));
    req.set_country(ctx->normalization_msg().country());
    req.set_appid(ctx->normalization_msg().appid());
    req.set_create_time(std::to_string(ctx->normalization_msg().reporttime()));
    req.add_models("likee-img-face-audit"); // storename
    if (ctx->normalization_msg().data().pic_size()) {
        req.set_image(ctx->raw_images_size() > 0 ? ctx->raw_images(0) : "");
    } else {
        LOG(ERROR) << "Call LikeeFaceAuditSubModule with no pic.";
        return false;
    }

    // uid 白名单策略    
    if (whitelist_uids_.size() == 0) { // 没有白名单uid，则推人审
        enable_review_ = true;
    } else if (whitelist_uids_.count(req.uid())) {  // 有白名单uid，则对应uid才推人审
        enable_review_ = true;
    } else {
        enable_review_ = false;
    }

    auto channel = channel_manager_.get_channel(url_);

    success_ = false;
    std::string result_string;
    for (int retry = 0; !success_ && retry < max_retry_ + 1; ++retry) {
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
            return success_; // 正常返回: 203-无人脸, 205-postid已出现过.
        }

        auto resp_body = cntl.response_attachment().to_string();
        int status_code = 0;
        if (!(parse_proxy_sidecar_response(resp_body, storename_, result_string, status_code) &&
              status_code == 200)) {
            log_error("Failed to parse response or status code is not 200.", ctx->traceid());
            continue;
        }
        success_ = true;
    }

    if (success_) {
        LikeeFaceAuditResponse resp;
        json2pb::JsonToProtoMessage(result_string, &resp);
        if (resp.hit_postids_size() > 0) {
            LikeeFaceAuditReview likee_face_audit_review;
            for (auto hit_postid: resp.hit_postids()) {
                info_.clear();
                if (!feature_info_query(ctx, hit_postid.post_id())) {
                    success_ = false;
                } else {

                    likee_face_audit_review.add_data_infos(info_);
                }
                if (likee_face_audit_review.data_infos_size() >= 8) {
                    break;  // 返回最多8条素材信息；
                }
            }
            info_.clear();
            json2pb::ProtoMessageToJson(likee_face_audit_review, &info_);
        }
    } else {
        add_err_num(1);
    }
    return success_;
}

bool LikeeFaceAuditSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    batch_info_.clear();
    for (auto &c : ctx) {
        call_service(c);
        batch_info_.emplace_back(info_);
    }
    return true;
}

bool LikeeFaceAuditSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    CHECK_EQ(ctxs.size(), batch_info_.size());
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i], batch_info_[i]);
    }
    return true;
}

bool LikeeFaceAuditSubModule::post_process(ContextPtr &ctx) { return post_process(ctx, info_); }

bool LikeeFaceAuditSubModule::post_process(ContextPtr &ctx, const std::string &info) {
    if (!success_ || context_is_download_failed(ctx)) {
        set_context_detailmlresult_modelinfo(ctx, "likee_face_audit", MODEL_RESULT_PASS, info);
    } else if (info.empty()) {
        set_context_detailmlresult_modelinfo(ctx, "likee_face_audit", MODEL_RESULT_PASS, info);
    } else {
        add_hit_num(1);
        if (enable_review_) {
            set_context_detailmlresult_modelinfo(ctx, "likee_face_audit", MODEL_RESULT_REVIEW, info);
        } else {
            set_context_detailmlresult_modelinfo(ctx, "likee_face_audit", MODEL_RESULT_PASS, info);
        }
    }
    return true;
}

bool LikeeFaceAuditSubModule::feature_info_query(ContextPtr &ctx, const std::string &featue_id) {
    FeatureMatchInfoQueryRequest req;
    req.set_featureid1(featue_id);
    req.set_appid(ctx->normalization_msg().appid());
    req.set_storename(storename_);
    req.set_url(get_context_url(ctx));

    auto channel = channel_manager_.get_channel(query_url_);

    bool success = false;
    for (int retry = 0; !success && retry < query_max_retry_; ++retry) {
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
            // get clean data info:
            std::string str = "{}"; // 这个是必须的，且不能为""，否则Parse出错
            rapidjson::Document clean_json_obj;
            clean_json_obj.Parse(str.c_str());
            rapidjson::Document::AllocatorType &allocator = clean_json_obj.GetAllocator();
            clean_json_obj.AddMember("code", json_obj["code"], allocator);
            rapidjson::Value data(rapidjson::kObjectType);
            clean_json_obj.AddMember("data", data, allocator);
            if (data_obj.HasMember("id") && data_obj["id"].IsString() && data_obj["id"].Size() > 0)
                clean_json_obj["data"].AddMember("id", data_obj["id"], allocator);
            if (data_obj.HasMember("url") && data_obj["url"].IsString() && data_obj["url"].Size() > 0)
                clean_json_obj["data"].AddMember("url", data_obj["url"], allocator);               
            if (data_obj.HasMember("remark") && data_obj["remark"].IsString() && data_obj["remark"].Size() > 0)
                clean_json_obj["data"].AddMember("remark", data_obj["remark"], allocator);
            if (data_obj.HasMember("dealType") && data_obj["dealType"].IsInt() && data_obj["dealType"].GetInt() > 0)
                clean_json_obj["data"].AddMember("dealType", data_obj["dealType"], allocator);
            info_ = stringify(clean_json_obj);
            // info_ = resp_body; // full data info
        }
        success = true;
    }
    return success;
}

RegisterClass(SubModule, LikeeFaceAuditSubModule);