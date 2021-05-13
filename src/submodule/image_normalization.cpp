#include "image_normalization.h"

using namespace bigoai;

RegisterClass(SubModule, ImageNormalizationSubModule);

bool ImageNormalizationSubModule::init(const SubModuleConfig &conf) {
    url_ = conf.rpc().url();
    auto lb = conf.rpc().lb();
    auto retry_time = conf.rpc().max_retry();
    auto timeout_ms = conf.rpc().timeout_ms();

    channel_manager_.set_timeout_ms(timeout_ms);
    channel_manager_.set_retry_time(retry_time);
    channel_manager_.set_lb(lb);
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    models_.insert(models_.end(), conf.enable_model().begin(), conf.enable_model().end());

    CHECK_GE(models_.size(), 1u);
    return true;
}

bool ImageNormalizationSubModule::call_service(ContextPtr &ctx) {
    std::vector<ContextPtr> batch{ctx};
    return call_service_batch(batch);
}

bool ImageNormalizationSubModule::post_process(ContextPtr &ctx) {
    std::vector<ContextPtr> batch{ctx};
    return post_process_batch(batch);
}

bool ImageNormalizationSubModule::call_service_batch(std::vector<ContextPtr> &ctxs) {
    resp_.Clear();
    Request req;
    std::string id;

    for (auto ctx : ctxs) {
        CHECK_EQ(ctx->raw_images_size(), 1);
        *req.add_images() = ctx->raw_images(0);
        id += ctx->traceid() + "_";
    }

    req.set_id(id);
    auto url = url_;
    auto channel = channel_manager_.get_channel(url);
    if (channel == nullptr) {
        LOG(ERROR) << "Faield to init channel: " << url;
        return false;
    }

    brpc::Controller cntl;
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, &resp_, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call model service, " << cntl.ErrorText() << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment()
                   << ", status code: " << cntl.http_response().status_code()
                   << ", remote side: " << cntl.remote_side();
        return false;
    }

    return true;
}

bool ImageNormalizationSubModule::post_process_batch(std::vector<ContextPtr> &ctx_batch) {
    // When miss a model result, the all model is failed.
    if ((int)models_.size() != resp_.results_size()) {
        for (auto m : models_) {
            for (auto ctx : ctx_batch) {
                CHECK_EQ(ctx->normalization_msg().data().pic_size(), 1);
                auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0);
                (*image->mutable_detailmlresult())[m] = MODEL_RESULT_FAIL;
            }
        }
        add_err_num(ctx_batch.size());
        std::string debug;
        json2pb::ProtoMessageToJson(resp_, &debug);
        LOG(INFO) << "Failed to process response. The number of models not equal to " << models_.size()
                  << ", resp: " << debug;
        return false;
    }

    // When miss a image result, the all image is failed.
    for (size_t i = 0; i < ctx_batch.size(); ++i) {
        for (const auto &it : resp_.results()) {
            const auto model_res = it.second;
            // model_res.score_size must equal to batch size.
            if (model_res.score_size() != (int)ctx_batch.size()) {
                add_err_num(ctx_batch.size());
                std::string debug;
                json2pb::ProtoMessageToJson(resp_, &debug);
                LOG(INFO) << "Failed to process response. The response batch size not equal to " << ctx_batch.size()
                          << ", resp: " << debug;
                return false;
            }
        }
    }

    for (size_t i = 0; i < ctx_batch.size(); ++i) {
        CHECK_EQ(ctx_batch[i]->mutable_normalization_msg()->mutable_data()->pic_size(), 1);
        auto image = ctx_batch[i]->mutable_normalization_msg()->mutable_data()->mutable_pic(0);
        std::string country = ctx_batch[i]->normalization_msg().country();
        for (const auto &it : resp_.results()) {
            auto model_name = it.first;
            auto model_res = it.second;
            auto score = model_res.score(i);
            (*image->mutable_threshold())[model_name] = threshold_manager_.get_threshold(country, model_name);
            if (score > threshold_manager_.get_threshold(country, model_name)) {
                (*image->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
                add_hit_num(1);
            } else {
                (*image->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
            }
            ImageResponse::ImageModelRes model_res_single;
            model_res_single.add_score(score);
            std::string debug;
            json2pb::ProtoMessageToJson(model_res_single, &debug);
            (*image->mutable_modelinfo())[model_name] = debug;
        }
    }
    return true;
}


bool ImageNormalizationSubModule::is_allow_type(MessageType type) {
    if (type == IMAGE)
        return true;
    return false;
}
