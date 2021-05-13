#include "submodule/image_core.h"


using namespace bigoai;

ImageCoreSubModule::ImageCoreSubModule() {}


bool ImageCoreSubModule::init(const SubModuleConfig &conf) {
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "image_core_url is empty!";
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

    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImageCoreSubModule::call_service_batch(std::vector<ContextPtr> &ctxs) {
    resp_.Clear();
    success_ = false;

    ImageCoreRequest req;
    for (auto &model_name : model_names_) {
        req.add_modules(model_name);
    }

    std::string id;
    for (auto &ctx : ctxs) {
        req.add_images(ctx->raw_images_size() > 0 ? ctx->raw_images(0) : "");
        req.add_urls(get_context_url(ctx));
        id += (id.empty() ? "" : "_") + ctx->traceid();
    }

    if (req.images_size() == 0) {
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

        json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp_);
        if (resp_.status() != 200) {
            log_error("status is not 200!", id);
            continue;
        }
        if (size_t(resp_.results_size()) != ctxs.size()) {
            log_error("len(results) != len(contexts). " + std::to_string(resp_.results_size()) + " vs " +
                      std::to_string(ctxs.size()));
            continue;
        }

        auto ok = true;
        for (size_t i = 0; ok && i < ctxs.size(); ++i) {
            auto ctx = ctxs[i];
            auto res = resp_.results(i);
            if (res.score_size() != res.module_name_size()) {
                log_error("response len(score) != len(model_name). " + std::to_string(res.score_size()) + " vs " +
                              std::to_string(res.module_name_size()),
                          id);
                ok = false;
            }
            if (size_t(res.score_size()) != model_names_.size()) {
                log_error("len(score) != len(model). " + std::to_string(res.score_size()) + " vs " +
                              std::to_string(model_names_.size()),
                          id);
                ok = false;
            }
        }

        if (ok) {
            success_ = true;
        }
    }

    if (!success_) {
        add_err_num(ctxs.size());
    }
    return success_;
}

bool ImageCoreSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    for (size_t i = 0; i < ctxs.size(); ++i) {
        auto &ctx = ctxs[i];
        if (!success_ || context_is_download_failed(ctx)) {
            for (auto &model_name : model_names_) {
                set_context_detailmlresult_modelinfo(ctx, model_name, MODEL_RESULT_FAIL, "");
            }
            continue;
        }

        auto res = resp_.results(i);
        std::string country = ctx->normalization_msg().country();
        for (int j = 0; j < res.module_name_size(); ++j) {
            auto result = MODEL_RESULT_PASS;
            std::string model_name = model_names_[j];
            float threshold = threshold_manager_.get_threshold(country, model_name);
            if (res.score(j) > threshold) {
                result = MODEL_RESULT_REVIEW;
                add_hit_num(1);
            }


            ModelInfo info;
            info.set_score(res.score(j));
            info.set_version(resp_.version());
            info.set_threshold(threshold);

            json2pb::Pb2JsonOptions options;
            options.always_print_primitive_fields = true;
            options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

            std::string info_str;
            json2pb::ProtoMessageToJson(info, &info_str, options);

            set_context_detailmlresult_modelinfo(ctx, model_names_[j], result, info_str);
        }
    }
    return true;
}

bool ImageCoreSubModule::call_service(ContextPtr &ctx) {
    std::vector<ContextPtr> ctxs{ctx};
    return call_service_batch(ctxs);
}

bool ImageCoreSubModule::post_process(ContextPtr &ctx) {
    std::vector<ContextPtr> ctxs{ctx};
    auto ret = post_process_batch(ctxs);
    ctx = ctxs[0];
    return ret;
}


RegisterClass(SubModule, ImageCoreSubModule);
