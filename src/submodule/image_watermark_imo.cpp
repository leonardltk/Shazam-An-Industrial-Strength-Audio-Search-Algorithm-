#include "submodule/image_watermark_imo.h"


using namespace bigoai;

ImageWatermarkImoSubModule::ImageWatermarkImoSubModule() {}

bool ImageWatermarkImoSubModule::init(const SubModuleConfig &conf) {
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "watermark_url is empty!";
        return false;
    }
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(ERROR) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    model_names_.insert(model_names_.end(), conf.enable_model().begin(), conf.enable_model().end());
    if (model_names_.size() == 0) {
        LOG(ERROR) << "Module[" << name()
                   << "] model_names or model_thresholds is not specified, or the length does not match!";
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImageWatermarkImoSubModule::call_service(ContextPtr &ctx) {
    resp_.clear();
    multi_success_ = 0;

    WatermarkImoRequest req;
    auto id = ctx->normalization_msg().extradata().at("resource_id");
    for (auto &model_name : model_names_) {
        req.add_module_name(model_name);
    }

    // picture input
    req.set_op("query");
    req.set_id(id);
    req.set_post_id(id);
    req.set_isimage(true);
    auto pic_size = ctx->normalization_msg().data().pic_size();
    // LOG(INFO) << "pic size: " << pic_size << " raw image size: " << ctx->raw_images_size();
    auto channel = channel_manager_.get_channel(url_);
    for (int i = 0; i < pic_size; ++i) {
        success_ = false;
        ImageWatermarkImoResponse resp;
        if (ctx->normalization_msg().data().pic(i).failed_module().size() != 0 || ctx->raw_images(i).size() == 0)
            continue;
        req.clear_image();
        req.set_image(ctx->raw_images(i));

        for (int retry = 0; !success_ && retry < max_retry_; ++retry) {
            brpc::Controller cntl;
            cntl.http_request().uri() = url_;
            cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
            channel->CallMethod(nullptr, &cntl, &req, nullptr, nullptr);
            if (cntl.Failed()) {
                log_request_failed(cntl, id, retry);
                continue;
            }
            log_response_body(cntl, id);
            // LOG(INFO) << cntl.response_attachment().to_string();
            json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp);
            // LOG(INFO) << cntl.http_response().status_code();
            // LOG(INFO) << resp.status();
            if (cntl.http_response().status_code() != 200 && cntl.http_response().status_code() != 203) {
                LOG(ERROR) << "status is not 200! " << id;
                continue;
            }
            success_ = true;
            multi_success_ += 1;
        }
        resp_.push_back(resp);

        if (!multi_success_) {
            add_err_num(1);
        }
    }
    return multi_success_;
}

bool ImageWatermarkImoSubModule::post_process(ContextPtr &ctx) {
    if (!multi_success_ || context_is_download_failed(ctx)) {
        LOG(ERROR) << "context download failed.";
    }
    for (unsigned int i = 0; i < resp_.size(); ++i) {
        auto res = resp_[i];
        // auto result_flag = MODEL_RESULT_PASS;
        // if (res.det_flag() == 1)
        //     result_flag = MODEL_RESULT_REVIEW;

        // dual with result
        WatermarkImoResponse result;
        result.set_post_id(res.post_id());
        result.set_det_flag(0);
        if (res.det_flag() == 1 && res.det_scores_size()) {
            for (auto score : res.det_scores())
                result.add_scores(score);
            for (auto name : res.det_cls_names())
                result.add_cls_names(name);
            result.set_det_flag(res.det_flag());
        }
        else if (res.retrieval_result_size()) {
            if (float(res.max_retrieval_scores(0)) > 0.92 || float(res.max_retrieval_scores(2)) > 0.92 || float(res.max_retrieval_scores(4)) > 0.92 ||
            float(res.max_retrieval_scores(1)) > 0.9 || float(res.max_retrieval_scores(3)) > 0.9)
                result.set_det_flag(1);
            for (int j = 0; j < res.retrieval_result_size(); ++j){
                result.add_scores(res.retrieval_result(j).score());
                result.add_cls_names(res.retrieval_result(j).clsname());
            }
        }


        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

        std::string info_str;
        json2pb::ProtoMessageToJson(result, &info_str, options);
        // LOG(INFO) << info_str;
        // LOG(INFO) << model_names_[0];
        ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_modelinfo()->insert(
            google::protobuf::MapPair<std::string, std::string>(model_names_[0], info_str));
    }
    return true;
}


RegisterClass(SubModule, ImageWatermarkImoSubModule);
