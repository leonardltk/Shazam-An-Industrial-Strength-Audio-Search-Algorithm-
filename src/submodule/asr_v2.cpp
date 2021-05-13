#include "submodule/asr_v2.h"

using namespace bigoai;

AsrV2SubModule::AsrV2SubModule() {}


bool AsrV2SubModule::init(const SubModuleConfig &conf) {
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    channel_manager_.set_retry_time(conf.rpc().max_retry());

    url_ = conf.rpc().url();
    nlp_url_ = conf.asr().nlp().rpc().url();
    max_retry_ = conf.rpc().max_retry();
    nlp_max_retry_ = conf.asr().nlp().rpc().max_retry();
    appid_ = conf.asr().nlp().appid();
    businessid_ = conf.asr().nlp().businessid();
    text_type_ = conf.asr().nlp().text_type();

    brpc::ChannelOptions options;
    options.protocol = "http";
    options.timeout_ms = conf.asr().nlp().rpc().timeout_ms();
    options.max_retry = conf.asr().nlp().rpc().max_retry();

    if (0 != nlp_channel_.Init(nlp_url_.c_str(), conf.asr().nlp().rpc().lb().c_str(), &options)) {
        LOG(ERROR) << "Failed to init nlp channel.";
        return false;
    }
    LOG(INFO) << "Init " << this->name();
    return true;
}


bool AsrV2SubModule::call_keywords(ContextPtr ctx, std::string asr_text, keywords::Response &response) {
    response.Clear();

    brpc::Controller cntl;
    cntl.http_request().uri() = nlp_url_;
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
    for (int i = 0; i < nlp_max_retry_; ++i) {
        nlp_channel_.CallMethod(NULL, &cntl, &req, &response, NULL);
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

bool AsrV2SubModule::call_service(ContextPtr &ctx) {
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
        LOG(ERROR) << "Fail to call asr_v2 service, " << cntl.ErrorText() << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment()
                   << ", remote: " << cntl.remote_side() << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    if (asr_resp_.text().size() == 0) {
        return true;
    }
    auto ret = call_keywords(ctx, asr_resp_.text(), word_resp_);

    if (!ret) {
        add_err_num(1);
    }
    return ret;
}

bool AsrV2SubModule::post_process(ContextPtr &ctx) {
    if (!need_asr_) {
        return true;
    }

    CHECK_EQ(ctx->normalization_msg().data().voice_size(), 1);
    auto voice = ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0);

    if (asr_resp_.text().size() == 0) {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_PASS;
        (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_PASS;
        return true;
    }

    std::string asr_debug;
    json2pb::ProtoMessageToJson(asr_resp_, &asr_debug);
    (*voice->mutable_modelinfo())["asr"] = asr_debug;
    if (asr_resp_.text().size() == 0) {
        return true;
    }
    std::string nlp_debug;
    json2pb::ProtoMessageToJson(word_resp_.nlp(), &nlp_debug);
    (*voice->mutable_modelinfo())["nlp"] = nlp_debug;
    std::string keyword_debug;
    json2pb::ProtoMessageToJson(word_resp_.sensitive(), &keyword_debug);
    (*voice->mutable_modelinfo())["keywords"] = keyword_debug;
    LOG(INFO) << "AsrV2 resp: " << asr_debug << "; nlp resp: " << nlp_debug << "; keywords resp: " << keyword_debug
              << " [ID:" << ctx->traceid() << "] ";

    if (asr_resp_.status() != 200) {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_FAIL;
        // (*voice->mutable_detailmlresult())["nlp"] = MODEL_RESULT_FAIL;
        return false;
    }

    if (word_resp_.sensitive().rule_list_size() == 0) {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_PASS;
    } else {
        (*voice->mutable_detailmlresult())["keywords"] = MODEL_RESULT_REVIEW;
    }

    return true;
}

RegisterClass(SubModule, AsrV2SubModule);
