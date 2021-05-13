#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class MultiImageSubModule : public SubModule {
public:
    std::string name() const override { return "NormalizationSubModule"; }

    bool call_service(ContextPtr &ctx) override {
        resp_.Clear();
        Request req;
        std::string id = ctx->traceid();
        CHECK_EQ(ctx->raw_images_size(), ctx->normalization_msg().data().pic_size());
        for (auto image : ctx->raw_images()) {
            *req.add_images() = image;
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
                       << ", code: " << cntl.ErrorCode() << ", resp: " << cntl.response_attachment();
            return false;
        }
        return true;
    }

    bool post_process(ContextPtr &ctx) override {
        for (auto m : models_) {
            CHECK_EQ(ctx->normalization_msg().data().pic_size(), ctx->raw_images_size());
            for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
                auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
                (*image->mutable_detailmlresult())[m] = MODEL_RESULT_FAIL;
            }
        }
        // When miss a model result, the all model is failed.
        if ((int)models_.size() != resp_.results_size()) {
            add_err_num(1);
            std::string debug;
            json2pb::ProtoMessageToJson(resp_, &debug);
            LOG(INFO) << "Failed to process response. The number of models not equal to " << models_.size()
                      << ", resp: " << debug;
            return false;
        }

        // When miss a image result, the all image is failed.
        for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
            for (const auto &it : resp_.results()) {
                const auto model_res = it.second;
                if (model_res.score_size() != ctx->normalization_msg().data().pic_size()) {
                    add_err_num(1);
                    std::string debug;
                    json2pb::ProtoMessageToJson(resp_, &debug);
                    LOG(INFO) << "Failed to process response. The response batch size not equal to "
                              << ctx->normalization_msg().data().pic_size() << ", resp: " << debug;
                    return false;
                }
            }
        }

        for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
            auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
            std::string country = ctx->normalization_msg().country();
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
                std::string debug;
                json2pb::ProtoMessageToJson(model_res, &debug);
                (*image->mutable_modelinfo())[model_name] = debug;
            }
        }
        return true;
    }

    bool init(const SubModuleConfig &) override;

    virtual bool is_allow_type(MessageType type) {
        if (type == IMAGE)
            return true;
        return false;
    }

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::vector<std::string> models_;
    std::string url_;

    ImageResponse resp_;
};

} // namespace bigoai
