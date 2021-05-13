#include "submodule/gulf_asr.h"

using namespace bigoai;

GulfAsrSubModule::GulfAsrSubModule() {}


bool GulfAsrSubModule::init(const SubModuleConfig &conf) {
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    channel_manager_.set_retry_time(conf.rpc().max_retry());

    url_ = conf.rpc().url();
    keyword_url_ = conf.asr().keyword().rpc().url();
    max_retry_ = conf.rpc().max_retry();
    keyword_max_retry_ = conf.asr().keyword().rpc().max_retry();
    appid_ = conf.asr().keyword().appid();
    businessid_ = conf.asr().keyword().businessid();
    text_type_ = conf.asr().keyword().text_type();

    brpc::ChannelOptions options;
    options.protocol = "http";
    options.timeout_ms = conf.asr().keyword().rpc().timeout_ms();
    options.max_retry = conf.asr().keyword().rpc().max_retry();

    if (0 != word_channel_.Init(keyword_url_.c_str(), conf.asr().keyword().rpc().lb().c_str(), &options)) {
        LOG(ERROR) << "Failed to init keyword channel.";
        return false;
    }
    LOG(INFO) << "Init " << this->name();
    return true;
}


bool GulfAsrSubModule::call_keywords(ContextPtr ctx, std::string asr_text, keywords::Response &word_response) {
    word_response.Clear();

    brpc::Controller cntl;
    cntl.http_request().uri() = keyword_url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);

    keywords::Request req;
    keywords::ConfigPb *confPtr = req.mutable_config();
    confPtr->set_search_by_keyword(true);
    confPtr->set_search_by_regular(true);
    confPtr->set_keyword_type(0);

    req.set_uid(ctx->normalization_msg().uid());
    req.set_seqid(ctx->traceid());
    req.set_country(ctx->normalization_msg().country());
    req.set_content(asr_text);
    req.set_appid(appid_);
    req.set_businessid(businessid_);
    req.set_type(text_type_);
    req.set_disable_sensitive(false);
    req.set_disable_nlp(true);
    bool flag = true;
    for (int i = 0; i < keyword_max_retry_; ++i) {
        word_channel_.CallMethod(NULL, &cntl, &req, &word_response, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Failed to call mlplat keywords service. error: " << cntl.ErrorText() << ", "
                       << cntl.response_attachment() << ", remote side: " << cntl.remote_side()
                       << "[ID:" << ctx->traceid() << "]";
            flag = false;
            continue;
        }
        flag = true;
        break;
    }
    return flag;
}

bool GulfAsrSubModule::call_service(ContextPtr &ctx) {
    need_asr_ = false;
    for (auto it : ctx->normalization_msg().data().voice(0).detailmlresult()) {
        auto value = it.second;
        if (it.first != "voice_activity" && it.first != "lid_arabic" && value != "pass" && value != "fail") {
            (*ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_modelinfo())["asr-ign"] =
                "porn";
            VLOG(10) << "Skip Asr model. Model: " << it.first << ", value: " << value;
            return true;
        }
    }

    /*
    std::string value =
        (*(ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_detailmlresult()))["lid_arabic"];
    
    if (value == "pass") {
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_modelinfo())["asr-ign"] = "empty";
        return true;
    }
    */

    bool has_lid_arabic = false;
    for (auto it : ctx->normalization_msg().data().voice(0).detailmlresult()) {
        auto value = it.second;
	if (it.first == "lid_arabic") {
	   has_lid_arabic = true;
	   if (value == "pass") {
	       (*ctx->mutable_normalization_msg()->mutable_data()
	              ->mutable_voice(0)->mutable_modelinfo())["asr-ign"] = "empty";
               return true;
	   }
	}
    }

    if (!has_lid_arabic)
      return true;

    if (ctx->pcm_audio().size() == 0) {
        LOG(ERROR) << "Miss pcm data. [TraceID:" << ctx->traceid() << "]";
        return false;
    }
    need_asr_ = true;

    asr_resp_.Clear();
    AudioRequest req;

    CHECK_EQ(ctx->normalization_msg().data().voice_size(), 1);
    req.set_data(ctx->pcm_audio());
    req.set_data_type(AudioRequest::PCM_S16_SR16000);
    req.set_id(ctx->traceid());
    req.set_url(ctx->url_audio());
    // 3.0 Call service
    brpc::Controller cntl;
    auto channel = channel_manager_.get_channel(url_);
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, &asr_resp_, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call asr service, " << cntl.ErrorText() << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment()
                   << ", remote: " << cntl.remote_side() << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    if (asr_resp_.text().size() == 0) {
        return true;
    }

    // Note: We need trans BW to Arabic
    std::string BW_text = asr_resp_.text();
    LOG(INFO) << "ASR BW output: " << BW_text << " [ID:" << ctx->traceid() << "] ";
    // std::string BW_text = "Al$hwh Aljnsyh AgtSAb";
    std::string arabic_text = convert_buckwalter_to_arabic(BW_text);
    LOG(INFO) << "ASR arabic output: " << arabic_text << " [ID:" << ctx->traceid() << "] ";
    asr_resp_.set_text(arabic_text);
    auto ret = call_keywords(ctx, arabic_text, word_resp_);

    if (!ret) {
        add_err_num(1);
    }
    return ret;
}

// enable rewrite.
bool GulfAsrSubModule::post_process(ContextPtr &ctx) {
    if (!need_asr_)
        return true;

    CHECK_EQ(ctx->normalization_msg().data().voice_size(), 1);
    auto voice = ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0);

    if (asr_resp_.text().size() == 0) {
        voice->mutable_detailmlresult()->insert(
            google::protobuf::MapPair<std::string, std::string>("keywords", MODEL_RESULT_PASS));
        return true;
    }

    std::string asr_debug;
    json2pb::ProtoMessageToJson(asr_resp_, &asr_debug);
    voice->mutable_modelinfo()->insert(google::protobuf::MapPair<std::string, std::string>("asr", asr_debug));
    LOG(INFO) << "Asr resp: " << asr_debug << " [ID:" << ctx->traceid() << "] ";
    if (asr_resp_.text().size() == 0) {
        return true;
    }

    std::string keyword_debug;

    if (asr_resp_.status() != 200) {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_FAIL;
        json2pb::ProtoMessageToJson(word_resp_.sensitive(), &keyword_debug);
        (*voice->mutable_modelinfo())["keywords"] = keyword_debug;
        LOG(INFO) << "Words resp: " << keyword_debug << " [ID:" << ctx->traceid() << "] ";
        return false;
    }

    if (word_resp_.sensitive().rule_list_size() == 0) {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_PASS;
        json2pb::ProtoMessageToJson(word_resp_.sensitive(), &keyword_debug);
    } else {
        float total_score = 0.0;
        std::string arabic_text = asr_resp_.text();
        const char *p = arabic_text.c_str();
        size_t char_len = arabic_text.length();
        std::vector<int> alive_vector;
        for (int j = 0; j < word_resp_.sensitive().rule_list_size(); ++j) {
            auto rule_resp_ptr = (&word_resp_)->mutable_sensitive()->mutable_rule_list(j);
            size_t c_start = rule_resp_ptr->start();
            size_t c_end = rule_resp_ptr->end();
            bool should_delete = true;
            if ((c_start == 0) || (c_start > 0 && (*(p + c_start - 1) - ' ' == 0))) {
                if ((c_end == char_len) || (c_end < char_len && (*(p + c_end) - ' ' == 0))) {
                    should_delete = false;
                }
            }
            if (should_delete) {
                std::string rule = rule_resp_ptr->rule();
                std::string text = rule_resp_ptr->text();
                float score = 1.0 / (std::atof(rule_resp_ptr->extra_data().weight().c_str()));
                LOG(INFO) << "Delete detail: (" << j << ") [rule: " << rule << "] [text: " << text
                          << "] [score: " << score << "]  [ID:" << ctx->traceid() << "] ";
                continue;
            }

            bool should_add = true;
            for (size_t i = 0; i < alive_vector.size(); i++) {
                size_t alive_start = (&word_resp_)->mutable_sensitive()->mutable_rule_list(alive_vector[i])->start();
                size_t alive_end = (&word_resp_)->mutable_sensitive()->mutable_rule_list(alive_vector[i])->end();
                if (c_start <= alive_start && c_end >= alive_end) { // use new start and end
                    alive_vector[i] = j;
                    should_add = false;
                    break;
                } else if (c_start >= alive_start && c_end <= alive_end) { // not use
                    should_add = false;
                    break;
                }
            }
            if (should_add) {
                alive_vector.push_back(j);
            }
        }

        keywords::SensitiveWordResponse sensitive_response;
        sensitive_response.CopyFrom(word_resp_.sensitive());
        (&sensitive_response)->mutable_rule_list()->Clear();
        sensitive_response.set_is_hit(false);

        for (size_t i = 0; i < alive_vector.size(); i++) {
            auto rule_resp_ptr = (&word_resp_)->mutable_sensitive()->mutable_rule_list(alive_vector[i]);
            keywords::SensitiveWordHitRule *rule_temp = sensitive_response.add_rule_list();
            rule_temp->CopyFrom(word_resp_.sensitive().rule_list(alive_vector[i]));
            sensitive_response.set_is_hit(true);
            float score = 1.0 / (std::atof(rule_resp_ptr->extra_data().weight().c_str()));
            total_score += score;
            std::string rule = rule_resp_ptr->rule();
            std::string text = rule_resp_ptr->text();
            LOG(INFO) << "Hit detail: (" << alive_vector[i] << ") [rule: " << rule << "] [text: " << text
                      << "] [score: " << score << "]  [ID:" << ctx->traceid() << "] ";
        }

        if (total_score > 0.9999) {
            (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_REVIEW;
            add_hit_num(1);
            LOG(INFO) << "Must be review:"
                      << " [ID:" << ctx->traceid() << "] ";
        } else {
            (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_PASS;
        }
        json2pb::ProtoMessageToJson(sensitive_response, &keyword_debug);
    }

    (*voice->mutable_modelinfo())["keywords"] = keyword_debug;
    LOG(INFO) << "Words resp: " << keyword_debug << " [ID:" << ctx->traceid() << "] ";
    return true;
}

std::string GulfAsrSubModule::convert_buckwalter_to_arabic(const std::string &buckwalter) {
    std::wstring arabic_w;
    for (char c : buckwalter) {
        if (c - ' ' == 0) {
            arabic_w += L' ';
        } else {
            if (buckwalter_to_arabic.find(c) == buckwalter_to_arabic.end()) {
                LOG(INFO) << "BW2Arabic some error happen!"
                          << "error char: " << c;
            } else {
                arabic_w += buckwalter_to_arabic.at(c);
            }
        }
    }
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string arabic = converter.to_bytes(arabic_w);
    return arabic;
}

RegisterClass(SubModule, GulfAsrSubModule);
