#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/like_video/like_safe_audit.h"
#include "utils/like_effect.h"


using namespace bigoai;

bool LikeSafeAuditSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());

    for (auto it : conf.effect_list()) {
        std::string effect_name = it.effect_name();
        std::string model_name = it.model_name();
        if (model_name.size() == 0) {
            LOG(ERROR) << name() << ".effect_list." << effect_name << ".$model_name is empty";
            return false;
        }
        effect_blacklist_[model_name][effect_name] = {};
        effect_blacklist_[model_name][effect_name].insert(effect_blacklist_[model_name][effect_name].end(),
                                                          it.ids().begin(), it.ids().end());
    }

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();

    if (conf.enable_model_size() < 1) {
        LOG(ERROR) << "Failed to parse enable model field.";
        return false;
    } else {
        for (auto enable_model_it : conf.enable_model())
            enable_model_.push_back(enable_model_it);
    }

    response_version_ = conf.response_version();
    return true;
}

bool LikeSafeAuditSubModule::call_service(ContextPtr& ctx) {
    skip_ = true;
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    if (ctx->raw_video().size() == 0) {
        LOG(INFO) << "Empty data. [ID:" << ctx->traceid() << "]";
        return true;
    }

    /* if fine grain hit effect, return directly without calling model server */
    if (response_version_ == std::string("FineGrainedModelResponse") && effect_blacklist_.size() > 0) {
        filter_reason_ = ignore_model_by_effect((effect_blacklist_.begin())->second, ctx);
        if (filter_reason_.size()) {
            VLOG(10) << "Ignore SubModule[" << name() << "]: " << filter_reason_;
            return true;
        }
    }

    bool is_duet = ctx->is_duet();
    Request req;
    resp_fine_grained_.Clear();
    resp_tag_.Clear();
    req.set_video_url(ctx->normalization_msg().data().video(0).content());
    req.set_is_duet(is_duet);
    req.set_id(ctx->traceid());

    skip_ = false;

    // 3.0 Call service
    bool ret = false;
    if (response_version_ == std::string("FineGrainedModelResponse")) {
        ret = call_http_service(ctx, channel_manager_, url_, req, resp_fine_grained_, max_retry_, true);
    } else if (response_version_ == std::string("TaggingServiceResponse")) {
        ret = call_http_service(ctx, channel_manager_, url_, req, resp_tag_, max_retry_, true);
    }

    if (!ret)
        add_err_num();
    return ret;
}

// enable rewrite.
bool LikeSafeAuditSubModule::post_process(ContextPtr& ctx) {
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);

    if (response_version_ == std::string("FineGrainedModelResponse")) {  // for fine grained response
        if (filter_reason_.size()) {
            (*video->mutable_detailmlresult())[enable_model_[0]] = MODEL_RESULT_PASS;
            (*video->mutable_modelinfo())[enable_model_[0]] = filter_reason_;
            (*ctx->mutable_normalization_msg()->mutable_extradata())["effect_filter"] =
                enable_model_[0] + "/" + filter_reason_;
            (*ctx->mutable_model_score())[enable_model_[0]] = std::to_string(-2.0);
            return true;
        }

        if (skip_)
            return true;

        (*video->mutable_detailmlresult())[enable_model_[0]] = MODEL_RESULT_FAIL;
        std::string ret_str;
        json2pb::ProtoMessageToJson(resp_fine_grained_, &ret_str);
        (*video->mutable_modelinfo())[enable_model_[0]] = ret_str;

        if (resp_fine_grained_.name() != enable_model_[0]) {
            LOG(ERROR) << "Expect model: " << enable_model_[0] << " but get: " << resp_fine_grained_.name() << "[ID:" << ctx->traceid()
                       << "]";
            add_err_num();
            return false;
        }

        auto threshold = resp_fine_grained_.threshold();
        (*ctx->mutable_model_score())[enable_model_[0]] = std::to_string(resp_fine_grained_.score());
        if (resp_fine_grained_.score() > threshold) {
            (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
            (*video->mutable_detailmlresult())[enable_model_[0]] = MODEL_RESULT_REVIEW;
            add_hit_num();
        } else {
            (*video->mutable_detailmlresult())[enable_model_[0]] = MODEL_RESULT_PASS;
        }
        return true;
    } else if (response_version_ == std::string("TaggingServiceResponse")) {  // for tagging services
        if (skip_)
            return true;

        bool hit_flag = false;
        for (auto i = 0; i < resp_tag_.tags_size(); ++i) {
            auto model = resp_tag_.tags(i).name();
            std::string filter_reason;
            if (effect_blacklist_.size() > 0) {
                filter_reason = ignore_model_by_effect(effect_blacklist_[model], ctx);
                if (filter_reason.size() == 0) {
                    filter_reason = ignore_model_by_effect(effect_blacklist_["default"], ctx);
                }
                if (filter_reason.size()) {
                    VLOG(10) << "Ignore SubModule[" << name() << "]: " << filter_reason << ", model: " << model;
                    (*ctx->mutable_normalization_msg()->mutable_extradata())["effect_filter"] = model + "/" + filter_reason;
                    (*video->mutable_detailmlresult())[model] = MODEL_RESULT_PASS;
                    continue;
                }
            }
            
            (*video->mutable_detailmlresult())[model] = MODEL_RESULT_FAIL;
            std::string ret_str;
            json2pb::ProtoMessageToJson(resp_tag_.tags(i), &ret_str);
            (*video->mutable_modelinfo())[model] = ret_str;

            if (filter_reason.size()) {
                (*ctx->mutable_model_score())[model] = std::to_string(-2.0);
            } else {
                (*ctx->mutable_model_score())[model] = std::to_string(resp_tag_.tags(i).score());
            }

            auto threshold = resp_tag_.tags(i).threshold();
            if (resp_tag_.tags(i).score() > threshold) {
                (*video->mutable_detailmlresult())[model] = MODEL_RESULT_REVIEW;
                (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
                hit_flag = true;
            } else {
                (*video->mutable_detailmlresult())[model] = MODEL_RESULT_PASS;
            };
        }
        if (hit_flag) {
            add_hit_num();
        }
        return true;
    }
    return true;
}

RegisterClass(SubModule, LikeSafeAuditSubModule);
