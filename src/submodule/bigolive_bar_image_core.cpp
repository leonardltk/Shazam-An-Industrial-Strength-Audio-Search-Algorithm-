#include "submodule/bigolive_bar_image_core.h"


using namespace bigoai;

BigoliveBarImageCoreSubModule::BigoliveBarImageCoreSubModule() {}


bool BigoliveBarImageCoreSubModule::init(const SubModuleConfig &conf) {
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "image_core_url is empty!";
        return false;
    }
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    model_names_.insert(model_names_.end(), conf.enable_model().begin(), conf.enable_model().end());

    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool BigoliveBarImageCoreSubModule::call_service(ContextPtr &ctx) {
    resp_.Clear();
    success_ = false;

    ImageCoreRequest req;
    for (auto &model_name : model_names_) {
        req.add_modules(model_name);
    }

    std::string id = ctx->traceid();
    CHECK_EQ(ctx->raw_images_size(), ctx->normalization_msg().data().pic_size());
    VLOG(10) << "Raw_images_size : " << ctx->raw_images_size() << ", pic size : " << ctx->normalization_msg().data().pic_size();
    for (int i = 0; i < ctx->raw_images_size(); ++i) {
        req.add_images(ctx->raw_images(i));
        req.add_urls(ctx->normalization_msg().data().pic(i).content());
    }

    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(ERROR) << "Failed to init channel: " << url_;
        return false;
    }

    for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(nullptr, &cntl, &req, nullptr, nullptr);
        if (cntl.Failed()) {
            log_request_failed(cntl, id, retry);
            continue;
        }
        log_response_body(cntl, id);

        json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp_);
        if (resp_.status() != 200) {
            log_error("status is not 200!", id);
            continue;
        }
        if (resp_.results_size() != ctx->raw_images_size()) {
            log_error("len(results) != len(raw_images_size). " + std::to_string(resp_.results_size()) + " vs " +
                      std::to_string(ctx->raw_images_size()));
            continue;
        }
        success_ = true;
    }

    if (!success_) {
        add_err_num(ctx->raw_images_size());
    }
    return success_;
}

bool BigoliveBarImageCoreSubModule::post_process(ContextPtr &ctx) {

    for (auto m : model_names_) {
        CHECK_EQ(ctx->normalization_msg().data().pic_size(), ctx->raw_images_size());
        for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
            auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
            (*image->mutable_detailmlresult())[m] = MODEL_RESULT_FAIL;
        }
    }

    std::string country = ctx->normalization_msg().country();
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        auto res = resp_.results(i);
        auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);

        for (int j = 0; j < res.module_name_size(); ++j) {
            auto result = MODEL_RESULT_PASS;
            std::string model_name = model_names_[j];
            float threshold = threshold_manager_.get_threshold(country, model_name);
            if (res.score(j) > threshold) {
                result = MODEL_RESULT_REVIEW;
                add_hit_num(1);
            }

            ModelInfo info;
            info.set_score(res.score(j));
            info.set_version(resp_.version());
            info.set_threshold(threshold);

            json2pb::Pb2JsonOptions options;
            options.always_print_primitive_fields = true;
            options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

            std::string info_str;
            json2pb::ProtoMessageToJson(info, &info_str, options);
            if (image != nullptr) {
                (*image->mutable_detailmlresult())[model_name] = result;
                (*image->mutable_modelinfo())[model_name] = info_str;
            }
        }
    }
    return true;
}


RegisterClass(SubModule, BigoliveBarImageCoreSubModule);
