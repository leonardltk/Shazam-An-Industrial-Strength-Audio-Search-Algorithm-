#include "submodule/audio_core.h"

using namespace bigoai;

bool AudioCoreSubModule::init(const SubModuleConfig& conf) {
    models_.insert(models_.end(), conf.enable_model().begin(), conf.enable_model().end());

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

bool AudioCoreSubModule::call_service_batch(std::vector<ContextPtr>& ctx_batch) {
    // 2.0 Set request.
    resp_.Clear();
    Request req;
    std::string id;
    std::string url;
    for (size_t i = 0; i < ctx_batch.size(); ++i) {
        if (ctx_batch[i]->pcm_audio().size() == 0 || ctx_batch[i]->raw_audio().size() == 0) {
            LOG(INFO) << "Empty data. index: " << i << ", [ID:" << ctx_batch[i]->traceid() << "]";
            // continue;
        }
        *req.add_pcms() = ctx_batch[i]->pcm_audio();
        id += ctx_batch[i]->traceid() + "_";
        url += ctx_batch[i]->url_audio() + "_";
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
    if (channel == nullptr) {
        LOG(INFO) << "Failed to get channel. url: " << url_;
        return false;
    }
    bool flag = false;
    for (auto retry = 0; retry <= max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, &resp_, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Fail to call audio_core service, " << cntl.ErrorText().substr(0, 500) << "[" << name() << "]"
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
    VLOG(10) << "AudioCore: " << debug << "[ID: " << id << "]";
    return true;
}

bool AudioCoreSubModule::post_process_batch(std::vector<ContextPtr>& ctx_batch) {
    for (size_t i = 0; i < ctx_batch.size(); ++i) {
        for (size_t j = 0; j < models_.size(); ++j) {
            CHECK_EQ(ctx_batch[i]->mutable_normalization_msg()->mutable_data()->voice_size(), 1)
                << "[ID:" << ctx_batch[i]->traceid() << "]";
            auto model_name = models_[j];
            auto audio = ctx_batch[i]->mutable_normalization_msg()->mutable_data()->mutable_voice(0);
            (*audio->mutable_detailmlresult())[model_name] = "fail";
        }
    }

    if (ctx_batch.size() != (size_t)resp_.results_size()) {
        for (size_t i = 0; i < ctx_batch.size(); ++i) {
            LOG(ERROR) << "The number of response result not equal to " << ctx_batch.size() << "["
                       << resp_.results_size() << "] [ID:" << ctx_batch[i]->traceid() << "]";
        }
        add_err_num(ctx_batch.size());
        return false;
    }
    for (size_t i = 0; i < ctx_batch.size(); ++i) {
        CHECK_EQ(ctx_batch[i]->mutable_normalization_msg()->mutable_data()->voice_size(), 1);
        auto audio = ctx_batch[i]->mutable_normalization_msg()->mutable_data()->mutable_voice(0);
        std::string country = ctx_batch[i]->normalization_msg().country();
        if ((size_t)resp_.results(i).model_name_size() != models_.size()) {
            LOG(ERROR) << "The number of response model not equal to " << models_.size()
                       << "[ID:" << ctx_batch[i]->traceid() << "]";
            add_err_num(1);
            continue;
        }
        for (int j = 0; j < resp_.results(i).model_name_size(); ++j) {
            auto model_name = resp_.results(i).model_name(j);
            auto score = resp_.results(i).score(j);
            float threshold;
            if (resp_.results(i).threshold_size() > j && resp_.results(i).threshold(j) > 0) {
                threshold = resp_.results(i).threshold(j);
            } else {
                threshold = threshold_manager_.get_threshold(country, model_name);
            }
            (*audio->mutable_threshold())[model_name] = threshold;
            if (score > threshold) {
                (*audio->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
                if (model_name != "voice_activity") {
                    add_hit_num(1);
                }
            } else {
                (*audio->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
            }
            (*audio->mutable_modelinfo())[model_name] = std::to_string(score);
        }
        std::string debug;
        json2pb::ProtoMessageToJson(resp_.results(i), &debug);
        (*audio->mutable_modelinfo())["audio_core"] = debug;
    }
    return true;
}

bool AudioCoreSubModule::call_service(ContextPtr& ctx) {
    std::vector<ContextPtr> ctx_batch{ctx};
    return call_service_batch(ctx_batch);
}

bool AudioCoreSubModule::post_process(ContextPtr& ctx) {
    std::vector<ContextPtr> ctx_batch{ctx};
    auto ret = post_process_batch(ctx_batch);
    ctx = ctx_batch[0];
    return ret;
}

RegisterClass(SubModule, AudioCoreSubModule);
