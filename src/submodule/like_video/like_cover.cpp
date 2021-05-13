#include "submodule/like_video/like_cover.h"


using namespace bigoai;

bool LikeCoverSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();

    auto models = threshold_manager_.get_model_names();
    if (models.size() != 1) {
        std::ostringstream os;
        for (auto model : models)
            os << model << ",";
        LOG(ERROR) << "Except single model. Get: " << os.str();
        return false;
    }
    enable_model_ = models[0];
    return true;
}

bool LikeCoverSubModule::call_service(ContextPtr& ctx) {
    resp_.Clear();
    skip_ = true;
    Request req;
    if (ctx->raw_images_size() == 0 || ctx->raw_images(0).size() == 0) {
        LOG(ERROR) << "Miss cover content. [TraceID:" << ctx->traceid() << "]";
        return false;
    }

    req.set_cover(ctx->raw_images(0));
    req.set_post_id(ctx->traceid());
    req.set_url("kafka-consumer-cpp");
    req.add_run_model_names(enable_model_);
    req.add_run_model_thresholds(0);

    skip_ = false;
    if (false == call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_)) {
        LOG(ERROR) << "Failed to access blackscreen service after max retry [ID" << ctx->traceid() << "]";
        add_err_num();
        return false;
    }
    return true;
}

// enable rewrite.
bool LikeCoverSubModule::post_process(ContextPtr& ctx) {
    if (skip_)
        return true;
    auto cover = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0);
    std::string ret_str;
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*cover->mutable_modelinfo())[enable_model_] = ret_str;

    if (resp_.status() != 200) {
        return false;
    }
    (*cover->mutable_detailmlresult())[enable_model_] = MODEL_RESULT_FAIL;
    auto hit_flag = false;
    for (auto model : resp_.results()) {
        auto model_name = model.model_name();
        if (model_name != enable_model_)
            continue;
        auto threshold = threshold_manager_.get_threshold("default", model_name);
        if (model.raw_score_size()) {
            auto score = model.raw_score(0);
            (*ctx->mutable_model_score())[model_name] = std::to_string(score);
            if (score > threshold) {
                (*cover->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
                hit_flag = true;
                if (model_name == "black_screen") {
                    (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
                }
            } else {
                (*cover->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
            }
            (*cover->mutable_threshold())[model_name] = threshold;
        }
    }
    if (hit_flag)
        add_hit_num();
    return true;
}


RegisterClass(SubModule, LikeCoverSubModule);
