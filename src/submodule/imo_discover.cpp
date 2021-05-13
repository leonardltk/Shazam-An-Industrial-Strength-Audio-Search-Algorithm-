#include "submodule/imo_discover.h"

#include <algorithm>
//#include <chrono>
//#include <thread>

using namespace bigoai;
// using namespace std::literals::chrono_literals;

ImoDiscoverSubModule::ImoDiscoverSubModule() {}

bool ImoDiscoverSubModule::init(const SubModuleConfig &conf) {
    if (conf.extra_data_size() != 1 || conf.extra_data(0).key() != "result_key") {
        return false;
    }
    result_key_ = conf.extra_data(0).val();
    if (conf.rpc().url().size() == 0) {
        LOG(ERROR) << "imo_discover_url is empty!";
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

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ImoDiscoverSubModule::call_service(ContextPtr &ctx) {
    resps_.Clear();
    video_resps_.Clear();
    if (!context_data_check(ctx) || context_is_download_failed(ctx)) {
        return true;
    }

    if (!ctx->normalization_msg().data().video_size() && ctx->normalization_msg().data().pic_size() == 0)
        return true;

    auto resource_id = ctx->normalization_msg().extradata().at("resource_id");
    auto channel = channel_manager_.get_channel(url_);

    successes_.resize(ctx->normalization_msg().data().pic_size(), false);
    video_successes_.resize(1, false);

    dup_flag = false;
    if (ctx->normalization_msg().data().video_size()) {
        auto detail = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->detailmlresult();
        if (detail.find("group_chat_vega") == detail.end()) { // no results available
            auto req_id = std::to_string(get_time_us());

            ImoGroupChatMatchRequest req;
            *req.add_post_ids() = req_id; // todo: req_id needs modification
            req.set_op("query_write");
            req.set_resource_id(resource_id);
            *req.add_urls() = ctx->normalization_msg().data().video(0).content();
            req.mutable_videoinfo()->set_country("Global");
            req.mutable_videoinfo()->set_create_time(std::to_string(ctx->normalization_msg().reporttime()));
            req.set_video(ctx->raw_video());

            brpc::Controller cntl;
            cntl.http_request().uri() = url_;
            cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
            channel->CallMethod(NULL, &cntl, &req, NULL, NULL);
            if (cntl.Failed()) {
                LOG(ERROR) << "Fail to call imo discover service, " << cntl.ErrorText().substr(0, 500) << "[" << name()
                           << "]"
                           << ", code: " << cntl.ErrorCode();
                add_err_num(1);
                return false;
            }

            std::string result_string;
            auto resp_body = cntl.response_attachment().to_string();
            int status_code = 0;
            if (!(parse_proxy_sidecar_response(resp_body, result_key_, result_string, status_code) &&
                  status_code == 200)) {
                log_error("Failed to parse response or status code is not 200.", ctx->traceid());
                return false;
            }
            json2pb::JsonToProtoMessage(result_string, &video_resps_);
            video_successes_[0] = true;
        } else {
            dup_flag = true;
            return true;
        }
    }

    // request for image files
    ImoGroupChatMatchRequest req;
    if (!ctx->normalization_msg().data().video_size())
        req.set_resource_id(resource_id);
    req.set_op("query_write");
    req.mutable_videoinfo()->set_country("Global");
    req.mutable_videoinfo()->set_create_time(std::to_string(ctx->normalization_msg().reporttime()));
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        if (ctx->normalization_msg().data().pic(i).failed_module().size() != 0 || ctx->raw_images(i).size() == 0)
            continue;
        auto req_id = std::to_string(get_time_us());
        *req.add_post_ids() = req_id; // todo: req_id needs modification
        *req.add_urls() = ctx->normalization_msg().data().pic(i).content();
        *req.add_images() = ctx->raw_images(i);
    }

    brpc::Controller cntl;
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, &cntl, &req, NULL, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call imo discover service, " << cntl.ErrorText().substr(0, 500) << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode();
        add_err_num(1);
        return false;
    }

    std::string result_string;
    auto resp_body = cntl.response_attachment().to_string();
    VLOG(10) << resp_body;
    int status_code = 0;
    if (!(parse_proxy_sidecar_response(resp_body, result_key_, result_string, status_code) && status_code == 200)) {
        log_error("Failed to parse response or status code is not 200.", ctx->traceid());
        return false;
    }
    VLOG(10) << "Pic done. " << result_string;
    json2pb::JsonToProtoMessage(result_string, &resps_);
    // successes_.resize(ctx->normalization_msg().data().pic_size(), true);
    std::fill(successes_.begin(), successes_.end(), true);
    return true;
}

void ImoDiscoverSubModule::ParseDiscoverResult(const ImoDiscoverResponse &discvoer_resp,
                                               ImoDiscoverResponse &resultant_resp, const int index) {
    VLOG(10) << "Parse begin: " << index;
    if (discvoer_resp.results_size() > index)
        resultant_resp.set_result(discvoer_resp.results(index));
    if (discvoer_resp.distances_size() > index)
        resultant_resp.set_distance(discvoer_resp.distances(index));
    if (discvoer_resp.unique_ids_size() > index)
        resultant_resp.set_unique_id(discvoer_resp.unique_ids(index));
    if (discvoer_resp.group_ids_size() > index)
        resultant_resp.set_group_id(discvoer_resp.group_ids(index));
    if (discvoer_resp.info_status_size() > index)
        resultant_resp.set_video_status(discvoer_resp.info_status(index));
    if (discvoer_resp.group_attrs_size() > index) {
        *resultant_resp.mutable_group_attr() = discvoer_resp.group_attrs(index);
        *resultant_resp.mutable_group_reviews() = {discvoer_resp.group_attrs(index).group_reviews().begin(),
                                                   discvoer_resp.group_attrs(index).group_reviews().end()};
    }
    resultant_resp.mutable_group_attr()->clear_group_reviews();
    VLOG(10) << "Parse done: " << index;
}


bool ImoDiscoverSubModule::post_process(ContextPtr &ctx) {
    if (dup_flag)
        return true;
    // Process video results
    { // Process dup result
        std::string dup_str;
        ImoDiscoverResponse dup_resp;
        json2pb::Pb2JsonOptions options;
        options.always_print_primitive_fields = true;
        options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;
        if (ctx->normalization_msg().data().video_size()) {
            dup_resp.set_result(video_resps_.result());
            dup_resp.set_distance(video_resps_.distance());
            dup_resp.set_group_id(video_resps_.dup_group_id());
            json2pb::ProtoMessageToJson(dup_resp, &dup_str, options);
            ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo()->insert(
                google::protobuf::MapPair<std::string, std::string>("discover_dup_result", dup_str));
        } else {
            dup_resp.set_result(resps_.result());
            dup_resp.set_distance(resps_.distance());
            dup_resp.set_group_id(resps_.dup_group_id());
            json2pb::ProtoMessageToJson(dup_resp, &dup_str, options);
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0)->mutable_modelinfo()->insert(
                google::protobuf::MapPair<std::string, std::string>("discover_dup_result", dup_str));
        }
    }

    if (ctx->normalization_msg().data().video_size()) {
        if (video_resps_.results_size() != video_resps_.group_attrs_size()) {
            LOG(ERROR) << "Incomplete video response structure";
            return false;
        }
        if (video_successes_[0] == false) {
            ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("group_chat_vega", MODEL_RESULT_FAIL));
        } else {
            ImoDiscoverResponse video_resp;
            ParseDiscoverResult(video_resps_, video_resp, 0);
            if (video_resp.result() == video_resp.unique_id() || video_resp.distance() <= 0) {
                ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult()->insert(
                    google::protobuf::MapPair<std::string, std::string>("group_chat_vega", MODEL_RESULT_PASS));
            } else {
                json2pb::Pb2JsonOptions options;
                options.always_print_primitive_fields = true;
                options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

                std::string group_chat_vega_str;
                json2pb::ProtoMessageToJson(video_resp, &group_chat_vega_str, options);

                add_hit_num(1);
                ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult()->insert(
                    google::protobuf::MapPair<std::string, std::string>("group_chat_vega", MODEL_RESULT_REVIEW));
                ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo()->insert(
                    google::protobuf::MapPair<std::string, std::string>("group_chat_vega", group_chat_vega_str));
            }
        }
    }
    if (ctx->normalization_msg().data().pic_size() && (resps_.results_size() != resps_.group_attrs_size())) {
        LOG(ERROR) << "Incomplete pic response structure";
        return false;
    }
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        if (successes_[i] == false || ctx->normalization_msg().data().pic(i).failed_module().size() != 0 ||
            ctx->raw_images(i).size() == 0) {
            VLOG(10) << successes_[i] << " " << ctx->normalization_msg().data().pic(i).failed_module().size() << " "
                     << ctx->raw_images(i).size();
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("group_chat_vega", MODEL_RESULT_FAIL));
            continue;
        }
        ImoDiscoverResponse image_resp;
        ParseDiscoverResult(resps_, image_resp, i);
        if (image_resp.result() == image_resp.unique_id() || image_resp.distance() <= 0) {
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("group_chat_vega", MODEL_RESULT_PASS));
        } else {
            json2pb::Pb2JsonOptions options;
            options.always_print_primitive_fields = true;
            options.enum_option = json2pb::EnumOption::OUTPUT_ENUM_BY_NUMBER;

            std::string group_chat_vega_str;
            json2pb::ProtoMessageToJson(image_resp, &group_chat_vega_str, options);
            add_hit_num(1);
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_detailmlresult()->insert(
                google::protobuf::MapPair<std::string, std::string>("group_chat_vega", MODEL_RESULT_REVIEW));
            ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i)->mutable_modelinfo()->insert(
                google::protobuf::MapPair<std::string, std::string>("group_chat_vega", group_chat_vega_str));
        }
    }
    return true;
}

bool ImoDiscoverSubModule::post_process(ContextPtr &ctx, const ImoGroupChatMatchResponse &resp) { return true; }


RegisterClass(SubModule, ImoDiscoverSubModule);
