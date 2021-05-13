#include "submodule/image_watermark.h"


using namespace bigoai;

ImageWatermarkSubModule::ImageWatermarkSubModule() {}


bool ImageWatermarkSubModule::init(const SubModuleConfig &conf) {
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "watermark_url is empty!";
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

    model_names_.insert(model_names_.end(), conf.enable_model().begin(), conf.enable_model().end());
    if (model_names_.size() == 0) {
        LOG(ERROR) << "Module[" << name()
                   << "] model_names or model_thresholds is not specified, or the length does not match!";
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImageWatermarkSubModule::call_service_batch(std::vector<ContextPtr> &ctxs) {
    resp_.Clear();
    success_ = false;

    ImageWatermarkRequest req;
    for (auto &model_name : model_names_) {
        req.add_module_name(model_name);
    }

    std::string id;
    for (auto &ctx : ctxs) {
        auto pic_ptr = get_context_data_object(ctx);

        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string info_str;
        json2pb::ProtoMessageToJson(*pic_ptr, &info_str, options);
        LOG(INFO) << info_str;
        req.add_ocrs(info_str);
        req.add_urls(pic_ptr->content());
        id += (id.empty() ? "" : "_") + ctx->traceid();
    }

    if (req.ocrs_size() == 0) {
        return true;
    }

    auto channel = channel_manager_.get_channel(url_);

    for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(nullptr, &cntl, &req, nullptr, nullptr);
        if (cntl.Failed()) {
            log_request_failed(cntl, id, retry);
            continue;
        }
        log_response_body(cntl, id);
        std::cout << cntl.response_attachment().to_string() << std::endl;
        json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp_);
        if (resp_.status() != 200) {
            log_error("status is not 200!", id);
            continue;
        }
        if (size_t(resp_.frames_size()) != ctxs.size()) {
            log_error("len(cls_names_size) != len(contexts). " + std::to_string(resp_.frames_size()) + " vs " +
                      std::to_string(ctxs.size()));
            continue;
        }
        else
            success_ = true;
    }

    if (!success_) {
        add_err_num(ctxs.size());
    }
    return success_;
}

bool ImageWatermarkSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    for (size_t i = 0; i < ctxs.size(); ++i) {
        auto &ctx = ctxs[i];
        LOG(INFO) << ctx;
        if (!success_ || context_is_download_failed(ctx)) {
            for (auto &model_name : model_names_) {
                set_context_detailmlresult_modelinfo(ctx, model_name, MODEL_RESULT_FAIL, "");
            }
            continue;
        }

        auto res = resp_.frames(i);
        auto result = MODEL_RESULT_PASS;
        if (res.score() == 1)
            result = MODEL_RESULT_REVIEW;
            
        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string info_str;
        json2pb::ProtoMessageToJson(res, &info_str, options);
        LOG(INFO) << info_str;
        LOG(INFO) << model_names_[0];
        LOG(INFO) << result;
        set_context_detailmlresult_modelinfo(ctxs[i], model_names_[0], result, info_str);

    }
    return true;
}

bool ImageWatermarkSubModule::call_service(ContextPtr &ctx) {
    std::vector<ContextPtr> ctxs{ctx};
    return call_service_batch(ctxs);
}

bool ImageWatermarkSubModule::post_process(ContextPtr &ctx) {
    std::vector<ContextPtr> ctxs{ctx};
    auto ret = post_process_batch(ctxs);
    ctx = ctxs[0];
    return ret;
}


RegisterClass(SubModule, ImageWatermarkSubModule);
