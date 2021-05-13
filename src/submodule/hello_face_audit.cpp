//#include <random>

#include "hello_face_audit.h"

using namespace bigoai;
RegisterClass(SubModule, HelloFaceAuditSubModule);

bool HelloFaceAuditSubModule::init(const SubModuleConfig &conf) {
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool HelloFaceAuditSubModule::call_service(ContextPtr &ctx) {
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    FaceAuditRequest req;
    req.set_op("query_write");
    req.set_uid(ctx->normalization_msg().uid());
    req.set_url(get_context_url(ctx));
    req.set_country(ctx->normalization_msg().country());
    req.set_business(ctx->normalization_msg().businesstype());
    req.set_create_time(std::to_string(ctx->normalization_msg().reporttime()));
    if (ctx->normalization_msg().data().pic_size()) {
        req.set_image(ctx->raw_images_size() > 0 ? ctx->raw_images(0) : "");
    } else {
        LOG(ERROR) << "Call HelloFaceAuditSubModule with no pic.";
        return false;
    }

    auto channel = channel_manager_.get_channel(url_);

    success_ = false;
    std::string result_string;
    for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
        // 3.0 Call service
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, &face_audit_pb_, NULL);
        std::string face_audit_pb_str;
        json2pb::ProtoMessageToJson(face_audit_pb_, &face_audit_pb_str);
        if (cntl.Failed()) {
            log_request_failed(cntl, ctx->traceid(), retry);
            LOG(ERROR) << "Calling face audit service failed. [uid:" << req.uid() << "], [url:" << req.url()
                       << "], [status_code:" << cntl.http_response().status_code()
                       << "], [resp: " << cntl.response_attachment().to_string() << "], [result: " << face_audit_pb_str
                       << "]";
            continue;
        }

        if (cntl.http_response().status_code() == 200 || cntl.http_response().status_code() == 203) {
            VLOG(10) << "Calling face audit service succeed. [uid:" << req.uid() << "], [url:" << req.url()
                     << "], [status_code:" << cntl.http_response().status_code()
                     << "], [resp: " << cntl.response_attachment().to_string() << "], [result: " << face_audit_pb_str
                     << "]";
            success_ = true;
            return success_;
        }
    }

    if (!success_) {
        add_err_num(1);
    }
    return success_;
}

bool HelloFaceAuditSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    for (auto &c : ctx) {
        call_service(c);
    }
    return true;
}

bool HelloFaceAuditSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i]);
    }
    return true;
}

bool HelloFaceAuditSubModule::post_process(ContextPtr &ctx) {
    if (success_ == false || context_is_download_failed(ctx)) {
        set_context_modelinfo(ctx, "hello_face_audit", "fail but pass.");
    } else {
        // std::string face_audit_pb_str;
        // json2pb::ProtoMessageToJson(face_audit_pb_, &face_audit_pb_str);
        set_context_modelinfo(ctx, "hello_face_audit", "succeed.");
    }
    return true;
}