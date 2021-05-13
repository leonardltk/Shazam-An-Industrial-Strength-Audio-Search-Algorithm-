#include "submodule/video_ocr.h"

using namespace bigoai;


VideoOcrSubModule::VideoOcrSubModule() {}


bool VideoOcrSubModule::init(const SubModuleConfig &conf) {
    if (!conf.has_video_ocr()) {
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

    auto conf_ocr = conf.video_ocr();
    result_key_ = conf_ocr.result_key();
    channel_manager_text_.set_timeout_ms(conf_ocr.rpc().timeout_ms());
    channel_manager_text_.set_lb(conf_ocr.rpc().lb());
    text_url_ = conf_ocr.rpc().url();
    auto text_channel = channel_manager_text_.get_channel(text_url_);
    if (text_channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << text_url_;
        return false;
    }

    appid_ = conf_ocr.appid();
    businessid_ = conf_ocr.businessid();
    text_type_ = conf_ocr.text_type();
    text_max_retry_ = conf_ocr.rpc().max_retry();
    text_url_ = conf_ocr.rpc().url();
    check_level_ = conf_ocr.check_level();

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool VideoOcrSubModule::call_service(ContextPtr &ctx) {
    success_ = false;
    ocr_results_.clear();

    OcrRequest req;

    if (ctx->raw_video().size() == 0) {
        return false;
    }
    req.set_video(ctx->raw_video());

    auto channel = channel_manager_.get_channel(url_);

    for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(nullptr, &cntl, &req, nullptr, nullptr);
        if (cntl.Failed()) {
            log_request_failed(cntl, ctx->traceid(), retry);
            continue;
        }
        log_response_body(cntl, ctx->traceid());
        auto resp_body = cntl.response_attachment().to_string();
        std::string result_string;
        int status_code;
        if (!(parse_proxy_sidecar_response(resp_body, result_key_, result_string, status_code) && status_code == 200)) {
            log_error("Failed to parse response or status code is not 200.", ctx->traceid());
            continue;
        }
        ImageOcrResponse resp;
        json2pb::JsonToProtoMessage(result_string, &resp);
        if (size_t(resp.results_size()) != 1) {
            log_error("len(ocr_results) != len(contexts). " + std::to_string(resp.results_size()) +
                      " vs 1[ID:" + ctx->traceid() + "]");
            continue;
        }

        ImageOcrResult ocr_result;
        ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_NORMAL);
        *ocr_result.mutable_ocr_result() = resp.results(0);
        if (resp.results(0).text().empty()) {
            ocr_result.set_ocr_status(ImageOcrResult_STATUS_OCR_EMPTY);
        } else {
            text_audit(ctx, resp.results(0).text(), ocr_result);
        }
        ocr_results_.emplace_back(ocr_result);
        success_ = true;
    }

    if (!success_) {
        add_err_num(1);
    }
    return success_;
}

bool VideoOcrSubModule::post_process(ContextPtr &ctx) {
    if (!success_ || context_is_download_failed(ctx)) {
        set_context_detailmlresult_modelinfo(ctx, "ocr", MODEL_RESULT_FAIL, "", "video" );
        return true;
    }

    if (ocr_results_.size() != 1) {
        LOG(ERROR) << "Ocr result not equal to 1.";
        return false;
    }

    auto ocr_result = ocr_results_[0];
    auto res = MODEL_RESULT_PASS;
    std::string ocr_result_str = "";

    if (!success_ || context_is_download_failed(ctx)) {
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
    set_context_detailmlresult_modelinfo(ctx, "ocr", res, ocr_result_str, "video");
    return true;
}


bool VideoOcrSubModule::text_audit(ContextPtr &ctx, const std::string &text, ImageOcrResult &ocr_result) {
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
            log_error("Failed to text audit! " + resp.sensitive().error(), ctx->traceid());
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

RegisterClass(SubModule, VideoOcrSubModule);
