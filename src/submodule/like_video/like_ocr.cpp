#include <brpc/channel.h>
#include "utils/common_utils.h"
#include "submodule/like_video/like_ocr.h"
#include "keywords_search_v2.pb.h"

using namespace bigoai;


bool LikeOcrSubModule::init(const SubModuleConfig &conf) {
    set_name(conf.instance_name());
    if (!conf.has_image_ocr()) {
        return false;
    }

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();

    keyword_url_ = conf.image_ocr().rpc().url();
    keyword_channel_manager_.set_timeout_ms(conf.image_ocr().rpc().timeout_ms());
    keyword_channel_manager_.set_lb(conf.image_ocr().rpc().lb());
    keyword_max_retry_ = conf.rpc().max_retry();
    return true;
}

bool LikeOcrSubModule::call_service(ContextPtr &ctx) {
    ocr_info_.Clear();
    sensitive_resp_.Clear();
    hit_rules_.clear();

    Request req;
    if (ctx->raw_images_size() == 0 || ctx->raw_images(0).size() == 0) {
        LOG(ERROR) << "Miss cover content. [TraceID:" << ctx->traceid() << "]";
        return false;
    }
    req.add_images(ctx->raw_images(0));

    std::vector<std::string> countries = {"RU", "UA", "BY", "MD", "KZ", "TJ", "KG", "UZ", "TM",
                                          "AM", "GE", "AZ", "LV", "LT", "EE", "BG", "RS"};
    if (std::find(countries.begin(), countries.end(), ctx->normalization_msg().country()) == countries.end()) {
        VLOG(10) << "not in specified countries, use cover for ocr";
    } else {
        req.set_video_url(ctx->normalization_msg().data().video(0).content());
        req.set_id(ctx->traceid());
    }
    req.set_post_id(ctx->traceid());

    // 3.0 Call service without retry.
    LikeOcrResponse ocr_resp;
    if (false == call_http_service(ctx, channel_manager_, url_, req, ocr_resp, max_retry_)) {
        return false;
    }
    for (auto it : ocr_resp.results()) {
        if (1 != it.second.results_size()) {
            LOG(ERROR) << "Failed to parse ocr results.";
            add_err_num();
            return false;
        }
        ocr_info_ = it.second.results(0);
    }

    if (parse_response(ocr_info_, ctx) != true) {
        return false;
    }
    return true;
}

// enable rewrite.
bool LikeOcrSubModule::post_process(ContextPtr &ctx) {
    auto ptr = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    std::vector<std::string> countries = {"RU", "UA", "BY", "MD", "KZ", "TJ", "KG", "UZ", "TM",
                                          "AM", "GE", "AZ", "LV", "LT", "EE", "BG", "RS"};
    if (std::find(countries.begin(), countries.end(), ctx->normalization_msg().country()) == countries.end()) {
        ptr = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0);
    }

    const std::string model_name = "cover_ocr";
    (*ptr->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
    std::string ret_str;
    json2pb::ProtoMessageToJson(ocr_info_, &ret_str);
    (*ptr->mutable_modelinfo())[model_name] = ret_str;
    SensitiveWordResponse tmp;
    (*ctx->mutable_model_score())[model_name] = "0";
    for (auto rule : hit_rules_) {
        *tmp.add_rule_list() = rule;
        (*ptr->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*ctx->mutable_model_score())[model_name] = "1";
    }
    ret_str.clear();
    json2pb::ProtoMessageToJson(tmp, &ret_str);
    if (hit_rules_.size()) {
        add_hit_num();
    }
    (*ptr->mutable_modelinfo())["keyword"] = ret_str;
    return true;
}


bool LikeOcrSubModule::parse_response(const LikeOcrInfo &response, ContextPtr &ctx) {
    if (response.text().size()) {
        if (false == call_keywords(response.text(), ctx)) {
            LOG(ERROR) << "Failed to call keywords service. [ID:" << ctx->traceid() << "]";
            add_err_num();
            return false;
        }
    }
    return true;
}

bool LikeOcrSubModule::call_keywords(std::string ocr_text, const ContextPtr &ctx) {
    const int text_max_len = 200;
    std::wstring ocr_wtext;
    try {
        ocr_wtext = str2ws(ocr_text);
    } catch (std::range_error &e) {
        LOG(ERROR) << "Failed to convert ocr_text, maybe invalid UTF8 string.Ptr [ID:" << ctx->traceid() << "]";
        return false;
    }

    for (uint32_t index = 0; index < ocr_wtext.length(); index += text_max_len) {
        std::string textStr = ws2str(ocr_wtext.substr(index, text_max_len));
        TextRequest req;
        req.set_seqid(ctx->traceid());
        req.set_uid(ctx->normalization_msg().uid());
        req.set_reporttime(time(NULL));
        req.set_content(textStr);
        req.set_country(ctx->normalization_msg().country());
        req.set_appid("48");
        req.set_businessid(22);
        req.set_type(27);
        req.set_disable_sensitive(false);
        req.set_disable_nlp(true);

        ConfigPb *conPtr = req.mutable_config();
        conPtr->set_search_by_keyword(true);
        conPtr->set_search_by_regular(true);
        conPtr->set_keyword_type(0);

        TextResponse resp;
        if (false == call_http_service(ctx, channel_manager_, keyword_url_, req, resp, keyword_max_retry_)) {
            return false;
        }
        const SensitiveWordResponse &sensitive_resp = resp.sensitive();
        bool isHit = sensitive_resp.is_hit();
        if (!isHit) {
            continue;
        } else {
            for (const auto &hitRule : sensitive_resp.rule_list()) {
                hit_rules_.push_back(hitRule);
            }
        }
        std::string ret_str;
        json2pb::ProtoMessageToJson(sensitive_resp, &ret_str);
        LOG(INFO) << "keyword resp: " << ret_str;
    }
    return true;
}

RegisterClass(SubModule, LikeOcrSubModule);