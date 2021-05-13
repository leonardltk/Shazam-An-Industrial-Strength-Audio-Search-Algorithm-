#include "submodule/lid_arabic.h"

using namespace bigoai;

bool LidArabicSubModule::init(const SubModuleConfig& conf) {
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    max_retry_ = conf.rpc().max_retry();
    url_ = conf.rpc().url();
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    LOG(INFO) << "Init " << this->name();
    return true;
}

LidArabicSubModule::LidArabicSubModule() {
    models_.push_back("lid_arabic"); // only support lid_arabic now
}

bool LidArabicSubModule::call_service_batch(std::vector<ContextPtr>& ctx_batch) {
    // 2.0 Set request.
    resp_.Clear();
    already_sent_.clear();
    Request req;
    std::string id;
    std::string url;
    for (size_t i = 0; i < ctx_batch.size(); ++i) {
        if (ctx_batch[i]->pcm_audio().size() == 0 || ctx_batch[i]->raw_audio().size() == 0) {
            LOG(INFO) << "Empty data. index: " << i << ", [ID:" << ctx_batch[i]->traceid();
            continue;
        }
        std::string country = ctx_batch[i]->normalization_msg().country();
        // only support lid_arabic now
        if (threshold_manager_.get_threshold(country, "lid_arabic") > 1) {
            LOG(INFO) << "Not Arabic data. index: " << i << ", [ID:" << ctx_batch[i]->traceid();
            continue;
        }
        *req.add_pcms() = ctx_batch[i]->pcm_audio();
        id += ctx_batch[i]->traceid() + "_";
        url += ctx_batch[i]->url_audio() + "_";
        already_sent_.emplace_back(i);
    }
    if (req.pcms_size() == 0) {
        LOG(ERROR) << "Empty pcms.";
        return true;
    }

    req.set_post_id(id);
    req.set_url(url);
    for (auto m : models_) {
        req.add_models(m);
    }

    // 3.0 Call service
    auto channel = channel_manager_.get_channel(url_);
    bool flag = false;
    for (auto retry = 0; retry <= max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, &resp_, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Fail to call lid_arabic service, " << cntl.ErrorText().substr(0, 500) << "[" << name() << "]"
                       << ", code: " << cntl.ErrorCode() << ", remote: " << cntl.remote_side() << ", retry: " << retry;
            continue;
        }
        flag = true;
        break;
    }
    if (false == flag) {
        add_err_num(1);
        return false;
    }

    std::string debug;
    json2pb::ProtoMessageToJson(resp_, &debug);
    VLOG(10) << "LidArabic: " << debug << "[ID: " << id << "]";
    return true;
}

bool LidArabicSubModule::post_process_batch(std::vector<ContextPtr>& ctx_batch) {
    for (size_t i = 0; i < ctx_batch.size(); ++i) {
	if (ctx_batch[i]->pcm_audio().size() == 0 || ctx_batch[i]->raw_audio().size() == 0)
	    continue;
        for (size_t j = 0; j < models_.size(); ++j) {
            CHECK_EQ(ctx_batch[i]->mutable_normalization_msg()->mutable_data()->voice_size(), 1);
            auto model_name = models_[j];
            auto audio = ctx_batch[i]->mutable_normalization_msg()->mutable_data()->mutable_voice(0);
            std::string country = ctx_batch[i]->normalization_msg().country();
            if (threshold_manager_.get_threshold(country, model_name) > 1)
                continue;
            (*audio->mutable_detailmlresult())[model_name] = "fail";
        }
    }

    if (already_sent_.size() != (size_t)resp_.results_size()) {
        for (size_t i = 0; i < already_sent_.size(); ++i) {
            LOG(ERROR) << "The number of response result not equal to " << already_sent_.size() << "["
                       << resp_.results_size() << "] [ID:" << ctx_batch[already_sent_[i]]->traceid() << "]";
        }
        add_err_num(already_sent_.size());
        return false;
    }
    for (size_t i = 0; i < already_sent_.size(); ++i) {
        CHECK_EQ(ctx_batch[already_sent_[i]]->mutable_normalization_msg()->mutable_data()->voice_size(), 1);
        auto audio = ctx_batch[already_sent_[i]]->mutable_normalization_msg()->mutable_data()->mutable_voice(0);
        std::string country = ctx_batch[already_sent_[i]]->normalization_msg().country();
        if ((size_t)resp_.results(i).model_name_size() != models_.size()) {
            LOG(ERROR) << "The number of response model not equal to " << models_.size()
                       << "[ID:" << ctx_batch[already_sent_[i]]->traceid() << "]";
            add_err_num(1);
            continue;
        }
        for (int j = 0; j < resp_.results(i).model_name_size(); ++j) {
            auto model_name = resp_.results(i).model_name(j);
            auto score = resp_.results(i).score(j);
            (*audio->mutable_threshold())[model_name] = threshold_manager_.get_threshold(country, model_name);
            if (score > threshold_manager_.get_threshold(country, model_name)) {
                (*audio->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
            } else {
                (*audio->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
            }
            std::string debug;
            json2pb::ProtoMessageToJson(resp_.results(i), &debug);
            (*audio->mutable_modelinfo())[model_name] = std::to_string(score);
        }
    }
    return true;
}

bool LidArabicSubModule::call_service(ContextPtr& ctx) {
    std::vector<ContextPtr> ctx_batch{ctx};
    return call_service_batch(ctx_batch);
}

bool LidArabicSubModule::post_process(ContextPtr& ctx) {
    std::vector<ContextPtr> ctx_batch{ctx};
    auto ret = post_process_batch(ctx_batch);
    ctx = ctx_batch[0];
    return ret;
}

RegisterClass(SubModule, LidArabicSubModule);
