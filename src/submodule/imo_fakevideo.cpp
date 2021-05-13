#include "submodule/imo_fakevideo.h"


using namespace bigoai;

ImoFakevideoSubModule::ImoFakevideoSubModule() {}

bool ImoFakevideoSubModule::init(const SubModuleConfig &conf) {
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "imo_fakevideo url is empty!";
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

bool ImoFakevideoSubModule::call_service(ContextPtr &ctx) {
    scores_.clear();
    multi_success_ = 0;
    
    auto id = ctx->normalization_msg().extradata().at("resource_id");
    auto channel = channel_manager_.get_channel(url_);
    if (ctx->normalization_msg().data().video_size()) { // pass
        return true;
    }
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        success_ = false;
        if (ctx->normalization_msg().data().pic(i).failed_module().size() != 0 || ctx->raw_images(i).size() == 0)
            continue;
        
        ImoFakevideoRequest req;
        ImoFakevideoResponse resp;
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
            json2pb::JsonToProtoMessage(cntl.response_attachment().to_string(), &resp);
            if (cntl.http_response().status_code() != 200) {
                LOG(ERROR) << "status is not 200! " << id;
                continue;
            }
            success_ = true;
            multi_success_ += 1;
        }
        scores_.push_back(resp.score());
        if (!success_) {
            add_err_num(1);
        }
    }
    return multi_success_;
}

bool ImoFakevideoSubModule::post_process(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().video_size()) { // pass
        return true;
    }
    if (!multi_success_ || context_is_download_failed(ctx)) {
        LOG(ERROR) << "context download failed.";
        return true;
    }
    for (size_t i = 0; i < scores_.size(); ++i) {
        std::string score_str = scores_[i];
        // float score = -1;
        // try {
        //     score = std::stof(score_str);
        // } catch(...) {
        //     LOG(ERROR) << "Failed to parse fakevideo score: " << score_str;
        // }
        ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_modelinfo()->insert(
            google::protobuf::MapPair<std::string, std::string>(model_names_[0], score_str));
    }
    return true;
}


RegisterClass(SubModule, ImoFakevideoSubModule);
