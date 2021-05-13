// Warning: The file is decrypted
#include "submodule/asr.h"

using namespace bigoai;

AsrSubModule::AsrSubModule() {}


// ASR 肯定会过敏感词，不一定会过nlp
bool AsrSubModule::init(const SubModuleConfig &conf) {
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    channel_manager_.set_retry_time(conf.rpc().max_retry());
    brpc::ChannelOptions options;
    options.protocol = "http";

    options.timeout_ms = conf.asr().keyword().rpc().timeout_ms();
    options.max_retry = conf.asr().keyword().rpc().max_retry();

    url_ = conf.rpc().url();
    nlp_url_ = conf.asr().nlp().rpc().url();
    punct_url_ = conf.asr().punct_rpc().url();
    max_retry_ = conf.rpc().max_retry();
    nlp_max_retry_ = conf.asr().nlp().rpc().max_retry();
    punct_max_retry_ = conf.asr().punct_rpc().max_retry();
    enable_punct_ = conf.asr().enable_punct();

    srcid_ = conf.asr().keyword().srcid();
    businessid_ = conf.asr().keyword().businessid();
    appid_ = conf.asr().keyword().appid();
    text_type_ = conf.asr().keyword().text_type();


    while (0 != keywords_channel_.Init(conf.asr().keyword().rpc().url().c_str(),
                                       conf.asr().keyword().rpc().lb().c_str(), &options)) {
        LOG(ERROR) << "Failed to init keywords channel. Retry again.";
        sleep(1);
    }

    if (nlp_url_.size()) {
        if (0 !=
            nlp_channel_.Init(conf.asr().nlp().rpc().url().c_str(), conf.asr().nlp().rpc().lb().c_str(), &options)) {
            LOG(ERROR) << "Failed to init nlp channel.";
            return false;
        }
    } else {
        LOG(WARNING) << "Disable nlp service, nlp url is empty.";
    }

    if (0 != punct_channel_.Init(conf.asr().punct_rpc().url().c_str(), conf.asr().punct_rpc().lb().c_str(), &options)) {
        LOG(ERROR) << "Failed to init punct channel.";
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}


bool AsrSubModule::call_punct(ContextPtr ctx, std::string ocr_text, keywords::PunctResponse &punct_response,
                              std::string &punct_keywords_text, std::string &punct_nlp_text) {
    punct_response.Clear();
    brpc::Controller cntl;
    cntl.http_request().uri() = punct_url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    keywords::PunctRequest punct_req;
    punct_req.set_uid(ctx->normalization_msg().uid());
    punct_req.set_seqid(ctx->traceid());
    punct_req.set_country(ctx->normalization_msg().country());
    punct_req.set_content(ocr_text);

    punct_channel_.CallMethod(NULL, &cntl, &punct_req, &punct_response, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Failed to call punct service. error: " << cntl.ErrorText() << ", " << cntl.response_attachment();
        return false;
    }
    std::string punct_text = punct_response.text();
    if (enable_punct_) {
        punct_nlp_text = punct_response.text();
    }

    return true;
}


bool AsrSubModule::call_keywords(ContextPtr ctx, std::string ocr_text, keywords::HitResultPb &keywords_response,
                                 keywords::Response &nlp_response, keywords::PunctResponse &punct_response,
                                 AsrResponse &asr_response) {
    keywords_response.Clear();
    nlp_response.Clear();
    std::string punct_keywords_text = ocr_text;
    std::string punct_nlp_text = ocr_text;
    if (enable_punct_) {
        auto ret = call_punct(ctx, ocr_text, punct_response, punct_keywords_text, punct_nlp_text);
        if (ret) {
            if (punct_response.text().size() >= ocr_text.size())
                asr_response.set_text(punct_response.text());
        }
    }


    const int text_max_len = 500;
    for (uint32_t index = 0; index < punct_keywords_text.length(); index += text_max_len) {
        std::string textStr = punct_keywords_text.substr(index, text_max_len);
        keywords::KeywordsSearch_Stub stub(&keywords_channel_);

        keywords::Info request;

        keywords::ConfigPb *confPtr = request.mutable_config();
        confPtr->set_search_by_keyword(true);
        confPtr->set_search_by_regular(true);
        confPtr->set_keyword_type(0);

        request.set_uid(ctx->normalization_msg().uid());
        request.set_country(ctx->normalization_msg().country());
        request.set_appid(std::stoi(appid_));
        request.set_srcid(srcid_);
        request.set_content(textStr);

        brpc::Controller cntl;
        stub.searchWithStatistics(&cntl, &request, &keywords_response, NULL);

        if (cntl.Failed()) {
            LOG(ERROR) << "Failed to call keyword service. error: " << cntl.ErrorText()
                       << ", remote side: " << cntl.remote_side() << "[ID:" << ctx->traceid() << "]";
            continue;
        }

        std::string keywords_debug;
        json2pb::ProtoMessageToJson(keywords_response, &keywords_debug);

        bool is_hit = keywords_response.is_hit();
        if (!is_hit) {
            continue;
        } else {
            LOG(INFO) << "Hit keywords [ID:" << ctx->traceid() << "]";
            break;
        }
    }

    if (nlp_url_.size() == 0) {
        LOG(WARNING) << "Disable nlp service, nlp url is empty.";
        return true;
    }

    keywords::Request nlp_req;
    nlp_req.set_uid(ctx->normalization_msg().uid());
    nlp_req.set_seqid(ctx->traceid());
    nlp_req.set_country(ctx->normalization_msg().country());
    nlp_req.set_content(punct_nlp_text);
    nlp_req.set_appid(appid_);
    nlp_req.set_businessid(businessid_);
    nlp_req.set_type(text_type_);
    keywords::ConfigPb config;
    *nlp_req.mutable_config() = config;
    nlp_req.set_disable_sensitive(true);
    nlp_req.set_disable_nlp(false);
    bool flag = true;
    for (int i = 0; i <= nlp_max_retry_; ++i) {
        brpc::Controller cntl;
        cntl.http_request().uri() = nlp_url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        nlp_channel_.CallMethod(NULL, &cntl, &nlp_req, &nlp_response, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Failed to call nlp service. error: " << cntl.ErrorText() << ", "
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

bool AsrSubModule::call_service(ContextPtr &ctx) {
    need_asr_ = false;
    for (auto it : ctx->normalization_msg().data().voice(0).detailmlresult()) {
        auto value = it.second;
        if (it.first != "voice_activity" && value != "pass" && value != "fail") {
            (*ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_modelinfo())["asr-ign"] =
                "porn";
            VLOG(10) << "Skip Asr model. Model: " << it.first << ", value: " << value;
            return true;
        }
    }
    std::string value = (*(ctx->mutable_normalization_msg()
                               ->mutable_data()
                               ->mutable_voice(0)
                               ->mutable_detailmlresult()))["voice_activity"];
    if (value == "pass") {
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_modelinfo())["asr-ign"] = "empty";
        return true;
    }

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
    auto ret = call_keywords(ctx, asr_resp_.text(), keywords_resp_, nlp_resp_, punct_resp_, asr_resp_);

    if (!ret) {
        add_err_num(1);
    }
    return ret;
}

// enable rewrite.
bool AsrSubModule::post_process(ContextPtr &ctx) {
    if (!need_asr_) {
        return true;
    }
    CHECK_EQ(ctx->normalization_msg().data().voice_size(), 1);
    auto voice = ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0);

    if (asr_resp_.text().size() == 0) {
        if (nlp_url_.size()) {
            (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_PASS;
        }
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_PASS;
        return true;
    }

    std::string asr_debug;
    json2pb::ProtoMessageToJson(asr_resp_, &asr_debug);
    (*voice->mutable_modelinfo())["asr"] = asr_debug;
    std::string keywords_debug;
    json2pb::ProtoMessageToJson(keywords_resp_, &keywords_debug);
    (*voice->mutable_modelinfo())["keywords"] = keywords_debug;
    std::string nlp_debug;
    if (nlp_url_.size()) {
        json2pb::ProtoMessageToJson(nlp_resp_, &nlp_debug);
        (*voice->mutable_modelinfo())["nlp"] = nlp_debug;
    }
    std::string punct_debug;
    json2pb::ProtoMessageToJson(punct_resp_, &punct_debug);
    (*voice->mutable_modelinfo())["punct"] = punct_debug;
    VLOG(10) << "Asr resp: " << asr_debug << "; keywords resp: " << keywords_debug << "; nlp resp: " << nlp_debug
             << "; Punct resp: " << punct_debug << "[ID:" << ctx->traceid() << "]";

    if (asr_resp_.status() != 200) {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_FAIL;
        if (nlp_url_.size()) {
            (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_FAIL;
        }
        return false;
    }

    if (keywords_resp_.is_hit()) {
        add_hit_num(1);
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_REVIEW;
    } else {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_PASS;
    }

    if (nlp_url_.size() == 0) {
        return true;
    }

    if (nlp_resp_.nlp().items_size() != 1) {
        (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_FAIL;
        LOG(ERROR) << "Error NLP item size: " << nlp_resp_.nlp().items_size() << "[ID:" << ctx->traceid() << "]";
        return false;
    }

    if (nlp_resp_.nlp().items(0).label() == 1) {
        add_hit_num(1);
        (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_REVIEW;
    } else {
        (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_PASS;
    }
    return true;
}

RegisterClass(SubModule, AsrSubModule);
