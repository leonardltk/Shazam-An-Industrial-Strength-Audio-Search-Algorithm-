#include "submodule/image_ocr.h"

using namespace bigoai;


ImageOcrSubModule::ImageOcrSubModule() {}


bool ImageOcrSubModule::init(const SubModuleConfig &conf) {
    if (!conf.has_image_ocr()) {
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

    result_key_ = conf.image_ocr().result_key();
    channel_manager_text_.set_timeout_ms(conf.image_ocr().rpc().timeout_ms());
    channel_manager_text_.set_lb(conf.image_ocr().rpc().lb());
    text_url_ = conf.image_ocr().rpc().url();
    auto text_channel = channel_manager_text_.get_channel(text_url_);
    if (text_channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << text_url_;
        return false;
    }

    appid_ = conf.image_ocr().appid();
    businessid_ = conf.image_ocr().businessid();
    text_type_ = conf.image_ocr().text_type();
    text_max_retry_ = conf.image_ocr().rpc().max_retry();
    text_url_ = conf.image_ocr().rpc().url();
    check_level_ = conf.image_ocr().check_level();

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImageOcrSubModule::call_service_batch(std::vector<ContextPtr> &ctxs) {
    success_ = false;
    ocr_results_.clear();

    OcrRequest req;

    std::string id;
    for (auto &ctx : ctxs) {
        req.add_images(ctx->raw_images_size() > 0 ? ctx->raw_images(0) : "");
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
        auto resp_body = cntl.response_attachment().to_string();
        std::string result_string;
        int status_code;
        if (!(parse_proxy_sidecar_response(resp_body, result_key_, result_string, status_code) && status_code == 200)) {
            log_error("Failed to parse response or status code is not 200.", id);
            continue;
        }
        ImageOcrResponse resp;
        json2pb::JsonToProtoMessage(result_string, &resp);
        if (size_t(resp.results_size()) != ctxs.size()) {
            log_error("len(ocr_results) != len(contexts). " + std::to_string(resp.results_size()) + " vs " +
                          std::to_string(ctxs.size()),
                      id);
            continue;
        }

        ImageOcrResult ocr_result;
        ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_NORMAL);
        for (size_t i = 0; i < ctxs.size(); ++i) {
            *ocr_result.mutable_ocr_result() = resp.results(i);
            if (resp.results(i).text().empty()) {
                ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_EMPTY);
            } else {
                text_audit(ctxs[i], resp.results(i).text(), ocr_result);
            }
            ocr_results_.emplace_back(ocr_result);
        }
        success_ = true;
    }

    if (!success_) {
        add_err_num(ctxs.size());
    }
    return success_;
}

bool ImageOcrSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    for (size_t i = 0; i < ctxs.size(); ++i) {
        auto &ctx = ctxs[i];
        if (!success_ || context_is_download_failed(ctx)) {
            set_context_detailmlresult_modelinfo(ctx, "ocr", MODEL_RESULT_FAIL, "");
            continue;
        }

        auto ocr_result = ocr_results_[i];
        auto res = MODEL_RESULT_PASS;
        std::string ocr_result_str = "";

        if (!success_ || context_is_download_failed(ctxs[i])) {
            res = MODEL_RESULT_FAIL;
        } else {
            if (ocr_result.ocr_status() == ImageOcrResult_STATUS_OCR_CRITICAL) {
                add_hit_num(1);
                res = MODEL_RESULT_BLOCK;
            } else if (ocr_result.ocr_status() == ImageOcrResult_STATUS_OCR_DANGER) {
                add_hit_num(1);
                res = MODEL_RESULT_REVIEW;
            } else if (ocr_result.ocr_status() == ImageOcrResult_STATUS_OCR_SYS_ERR) {
                res = MODEL_RESULT_FAIL;
            }

            json2pb::Pb2JsonOptions options;
            // options.always_print_primitive_fields = true;
            options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

            json2pb::ProtoMessageToJson(ocr_result, &ocr_result_str, options);
        }
        set_context_detailmlresult_modelinfo(ctxs[i], "ocr", res, ocr_result_str);
    }
    return true;
}

bool ImageOcrSubModule::call_service(ContextPtr &ctx) {
    std::vector<ContextPtr> ctxs{ctx};
    return call_service_batch(ctxs);
}

bool ImageOcrSubModule::post_process(ContextPtr &ctx) {
    std::vector<ContextPtr> ctxs{ctx};
    auto ret = post_process_batch(ctxs);
    ctx = ctxs[0];
    return ret;
}


bool ImageOcrSubModule::text_audit(ContextPtr &ctx, const std::string &text, ImageOcrResult &ocr_result) {
    TextRequest req;

    req.set_uid(ctx->normalization_msg().uid());
    req.set_seqid(ctx->traceid());
    req.set_reporttime(ctx->normalization_msg().reporttime());
    req.set_content(text);
    req.set_country(ctx->normalization_msg().country());
    req.set_appid(appid_);
    req.set_businessid(businessid_);
    req.set_type(text_type_);
    req.set_ext("");
    req.set_disable_sensitive(false);
    req.set_disable_nlp(true);

    ConfigPb *conPtr = req.mutable_config();
    conPtr->set_search_by_keyword(true);
    conPtr->set_search_by_regular(true);
    conPtr->set_keyword_type(0);

    auto channel = channel_manager_text_.get_channel(text_url_);

    bool success = false;
    for (int retry = 0; !success && retry <= text_max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = text_url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(nullptr, &cntl, &req, nullptr, nullptr);
        if (cntl.Failed()) {
            log_request_failed(cntl, ctx->traceid(), retry);
            continue;
        }
        log_response_body(cntl, ctx->traceid());

        TextResponse resp;
        json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp);

        if (resp.nlp().items_size() > 0) {
            *ocr_result.mutable_nlp_result() = resp.nlp().items(0);
        }
        *ocr_result.mutable_sensitive_result() = resp.sensitive();

        if (ocr_result.nlp_result().label() == 1) {
            ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_DANGER);
        }
        if (resp.sensitive().status() != 1) {
            std::string debug;
            json2pb::ProtoMessageToJson(resp, &debug);
            log_error("Failed to text audit! Response:" + debug, ctx->traceid());
            continue;
        }
        if (resp.sensitive().is_hit()) {
            ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_DANGER);
            if (check_level_) {
                for (const auto &hit_rule : resp.sensitive().rule_list()) {
                    if (hit_rule.level() == SensitiveWordHitRule_WORD_LEVEL_HIGH) {
                        ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_CRITICAL);
                        break;
                    }
                }
            }
        }
        success = true;
    }
    if (!success) {
        ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_SYS_ERR);
    }
    return success;
}

RegisterClass(SubModule, ImageOcrSubModule);
