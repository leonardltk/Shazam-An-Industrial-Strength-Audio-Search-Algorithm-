#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/like_video/like_tag.h"
#include "utils/like_effect.h"


using namespace bigoai;

bool LikeTagSubModule::init(const SubModuleConfig& conf) {
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
    return true;
}

bool LikeTagSubModule::call_service(ContextPtr& ctx) {
    skip_ = true;
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    if (ctx->raw_video().size() == 0) {
        LOG(INFO) << "Empty data. [ID:" << ctx->traceid() << "]";
        return true;
    }

    Request req;
    resp_.Clear();
    req.set_video_url(ctx->normalization_msg().data().video(0).content());

    req.set_id(ctx->traceid());

    // 3.0 Call service
    skip_ = false;
    if (false == call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_)) {
        add_err_num();
        return false;
    }
    return true;
}

// enable rewrite.
bool LikeTagSubModule::post_process(ContextPtr& ctx) {
    if (skip_)
        return true;


    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);

    bool hit_flag = false;
    for (auto i = 0; i < resp_.tags_size(); ++i) {
        auto model = resp_.tags(i).name();
        auto filter_reason = ignore_model_by_effect(effect_blacklist_[model], ctx);
        if (filter_reason.size() == 0) {
            filter_reason = ignore_model_by_effect(effect_blacklist_["default"], ctx);
        }
        if (filter_reason.size()) {
            VLOG(10) << "Ignore SubModule[" << name() << "]: " << filter_reason << ", model: " << model;
            (*ctx->mutable_normalization_msg()->mutable_extradata())["effect_filter"] = model + "/" + filter_reason;
            (*video->mutable_detailmlresult())[model] = MODEL_RESULT_PASS;
            continue;
        }

        (*video->mutable_detailmlresult())[model] = MODEL_RESULT_FAIL;
        std::string ret_str;
        json2pb::ProtoMessageToJson(resp_.tags(i), &ret_str);
        (*video->mutable_modelinfo())[model] = ret_str;

        auto threshold = resp_.tags(i).threshold();
        (*ctx->mutable_model_score())[model] = std::to_string(resp_.tags(i).score());
        if (resp_.tags(i).score() > threshold) {
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

RegisterClass(SubModule, LikeTagSubModule);
