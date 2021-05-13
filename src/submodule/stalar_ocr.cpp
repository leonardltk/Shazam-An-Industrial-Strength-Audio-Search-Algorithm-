#include "submodule/stalar_ocr.h"

using namespace bigoai;

bool StalarOcrSubModule::init(const SubModuleConfig &conf) {
    if (!conf.has_image_ocr()) {
        return false;
    }
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    channel_manager_text_.set_timeout_ms(conf.image_ocr().rpc().timeout_ms());
    channel_manager_text_.set_lb(conf.image_ocr().rpc().lb());
    text_url_ = conf.image_ocr().rpc().url();
    auto text_channel = channel_manager_text_.get_channel(text_url_);
    if (text_channel == nullptr) {
        LOG(INFO) << " Failed init channel! " << name() << " " << text_url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}


bool StalarOcrSubModule::call_keyword(const std::string &text, const std::string &uid, const std::string &country,
                                      const std::string &traceid, keywords::Response &resp) {
    auto channel = channel_manager_text_.get_channel(text_url_);
    if (channel == nullptr)
        return false;
    keywords::ConfigPb conf;
    conf.set_search_by_keyword(true);
    conf.set_search_by_regular(true);
    keywords::Request req;
    req.set_uid(uid);
    req.set_seqid(traceid);
    req.set_content(text);
    req.set_country(country);
    req.set_appid("82");
    req.set_businessid(22);
    req.set_type(82);
    *req.mutable_config() = conf;

    brpc::Controller cntl;
    cntl.http_request().uri() = text_url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, &resp, NULL);

    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call keywords service, " << cntl.ErrorText().substr(0, 500) << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode();
        return false;
    }

    return true;
}


bool StalarOcrSubModule::parse_ocr(const NormalizationOCRResponse &ocr_resp, const ContextPtr &ctx,
                                   keywords::Response &keyword_resp) {
    auto uid = ctx->normalization_msg().uid();
    auto cn = ctx->normalization_msg().country();
    auto traceid = ctx->traceid();
    for (auto it = ocr_resp.results().cbegin(); it != ocr_resp.results().cend(); ++it) {
        for (int i = 0; i < it->second.results_size(); ++i) {
            call_keyword(it->second.results(i).text(), uid, cn, traceid, keyword_resp);
            if (keyword_resp.sensitive().is_hit()) {
                std::string debug;
                json2pb::ProtoMessageToJson(keyword_resp, &debug);
                LOG(INFO) << "Hit keyword: " << debug << "[ID:" << ctx->traceid() << "]";
                break;
            }
        }
    }
    return true;
}

StalarOcrSubModule::StalarOcrSubModule() {}

bool StalarOcrSubModule::call_service(ContextPtr &ctx) {
    CHECK_EQ(ctx->raw_images_size(), ctx->normalization_msg().data().pic_size());
    ocr_resps_.clear();
    keyword_resps_.clear();
    ocr_resps_.resize(ctx->normalization_msg().data().pic_size());
    keyword_resps_.resize(ctx->normalization_msg().data().pic_size());

    auto channel = channel_manager_.get_channel(url_);
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        if (ctx->normalization_msg().data().pic(i).failed_module().size() != 0)
            continue;
        Request req;
        req.add_images(ctx->raw_images(i));
        req.set_post_id(ctx->traceid() + "_" + std::to_string(i));

        brpc::Controller cntl;
        cntl.http_request().uri() = url_;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        channel->CallMethod(NULL, &cntl, &req, &ocr_resps_[i], NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Fail to call normalization_ocr service, " << cntl.ErrorText().substr(0, 500) << "[" << name()
                       << "]"
                       << ", code: " << cntl.ErrorCode();
            add_err_num(1);
            continue;
        }
        if (false == parse_ocr(ocr_resps_[i], ctx, keyword_resps_[i])) {
            LOG(INFO) << "Failed to parse ocr text";
            add_err_num(1);
            continue;
        }

        std::string ocr_info;
        json2pb::ProtoMessageToJson(ocr_resps_[i], &ocr_info);
        std::string keyword_info;
        json2pb::ProtoMessageToJson(keyword_resps_[i], &keyword_info);
        LOG(INFO) << "ocr response: " << ocr_info << "; keyword: " << keyword_info << ", index: " << i
                  << ", all: " << ctx->normalization_msg().data().pic_size() << "[ID:" << ctx->traceid() << "]";
    };
    return true;
}

// enable rewrite.
bool StalarOcrSubModule::post_process(ContextPtr &ctx) {
    for (size_t i = 0; i < keyword_resps_.size(); ++i) {
        if (ctx->normalization_msg().data().pic(i).failed_module().size() != 0) {
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("ocr", MODEL_RESULT_FAIL));
        } else if (keyword_resps_[i].sensitive().is_hit()) {
            add_hit_num(1);
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("ocr", MODEL_RESULT_BLOCK));
        } else {
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("ocr", MODEL_RESULT_PASS));
        }

        std::string ocr_info;
        json2pb::ProtoMessageToJson(ocr_resps_[i], &ocr_info);
        ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_modelinfo()->insert(
            google::protobuf::MapPair<std::string, std::string>("ocr", ocr_info));
        std::string keyword_info;
        json2pb::ProtoMessageToJson(keyword_resps_[i], &keyword_info);
        ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_modelinfo()->insert(
            google::protobuf::MapPair<std::string, std::string>("keyword", keyword_info));
    }
    return true;
}

RegisterClass(SubModule, StalarOcrSubModule);
