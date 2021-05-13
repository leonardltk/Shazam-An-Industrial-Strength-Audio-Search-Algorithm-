#include "submodule/like_video/like_fine_grained.h"
#include "utils/like_effect.h"

using namespace bigoai;

bool FineGrainedSubModule::init(const SubModuleConfig &conf) {
    set_name(conf.instance_name());

    for (auto it : conf.effect_list()) {
        std::string effect_name = it.effect_name();
        effect_blacklist_[effect_name] = {};
        effect_blacklist_[effect_name].insert(effect_blacklist_[effect_name].end(), it.ids().begin(), it.ids().end());
    }
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();

    if (conf.enable_model_size() != 1) {
        LOG(ERROR) << "Failed to parse enable model field.";
        return false;
    }
    enable_model_ = conf.enable_model(0);
    return true;
}

bool FineGrainedSubModule::call_service(ContextPtr &ctx) {
    // 2.0 Set request.
    skip_ = true;
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    if (ctx->raw_video().size() == 0) {
        LOG(ERROR) << "Empty data. [ID:" << ctx->traceid() << "]";
        return true;
    }
    filter_reason_ = ignore_model_by_effect(effect_blacklist_, ctx);
    if (filter_reason_.size()) {
        VLOG(10) << "Ignore SubModule[" << name() << "]: " << filter_reason_;
        return true;
    }

    Request req;
    resp_.Clear();
    req.set_video_url(ctx->normalization_msg().data().video(0).content());

    bool is_duet = ctx->is_duet();
    req.set_is_duet(is_duet);
    req.set_id(ctx->traceid());

    skip_ = false;
    // 3.0 Call service
    auto ret = call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_, true);
    if (ret == false) {
        add_err_num();
    }
    return ret;
}

// enable rewrite.
bool FineGrainedSubModule::post_process(ContextPtr &ctx) {
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);

    if (filter_reason_.size()) {
        (*video->mutable_detailmlresult())[enable_model_] = MODEL_RESULT_PASS;
        (*video->mutable_modelinfo())[enable_model_] = filter_reason_;
        (*ctx->mutable_normalization_msg()->mutable_extradata())["effect_filter"] =
            enable_model_ + "/" + filter_reason_;
        return true;
    }
    if (skip_)
        return true;

    (*video->mutable_detailmlresult())[enable_model_] = MODEL_RESULT_FAIL;
    std::string ret_str;
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*video->mutable_modelinfo())[enable_model_] = ret_str;

    if (resp_.name() != enable_model_) {
        LOG(ERROR) << "Expect model: " << enable_model_ << " but get: " << resp_.name() << "[ID:" << ctx->traceid()
                   << "]";
        add_err_num();
        return false;
    }

    auto threshold = resp_.threshold();
    (*ctx->mutable_model_score())[enable_model_] = std::to_string(resp_.score());
    if (resp_.score() > threshold) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*video->mutable_detailmlresult())[enable_model_] = MODEL_RESULT_REVIEW;
        add_hit_num();
    } else {
        (*video->mutable_detailmlresult())[enable_model_] = MODEL_RESULT_PASS;
    }
    return true;
}

RegisterClass(SubModule, FineGrainedSubModule);
