#include "submodule/like_video/like_general_feature.h"


namespace bigoai {

RegisterClass(SubModule, LikeGeneralFeatureModule);

static uint64_t GetUidByVid(const std::string &vid) { return atoll(vid.c_str()) & (uint64_t)0xFFFFFFFF; }

bool LikeGeneralFeatureModule::init(const SubModuleConfig &conf) {
    set_name(conf.instance_name());
    if (conf.extra_data_size() != 4 || conf.extra_data(0).key() != "ann_url" ||
        conf.extra_data(1).key() != "kakashi_url" || conf.extra_data(2).key() != "lowq_url" ||
        conf.extra_data(3).key() != "ann_blacklist_url") {
        LOG(INFO) << "Failed to parse extra data field.";
        return false;
    }

    ann_url_ = conf.extra_data(0).val();
    kakashi_url_ = conf.extra_data(1).val();
    lowq_url_ = conf.extra_data(2).val();
    ann_blacklist_url_ = conf.extra_data(3).val();

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}


template<class REQUEST, class RESPONSE>
std::shared_ptr<brpc::Controller> call_http_service_async(const ContextPtr &ctx, ChannelManager &channel_manager,
                                                          const std::string &url, const REQUEST &req, RESPONSE &resp) {
    auto channel = channel_manager.get_channel(url);
    if (channel == nullptr) {
        LOG(FATAL) << "Failed to get channel. url: " << url << "[ID:" << ctx->traceid() << "]";
        return nullptr;
    }
    std::shared_ptr<brpc::Controller> cntl(new brpc::Controller());
    cntl->http_request().uri() = url;
    cntl->http_request().set_method(brpc::HTTP_METHOD_POST);
    channel->CallMethod(NULL, cntl.get(), &req, &resp, brpc::DoNothing());
    return cntl;
}


bool LikeGeneralFeatureModule::call_service(ContextPtr &ctx) {
    skip_ = true;
    if (ctx->video_src() != 4 && ctx->video_src() != 0) {
        VLOG(10) << "Skip video NOT uploaded from local DCIM(src!=4|0), src: " << ctx->video_src()
                 << ". [ID:" << ctx->traceid() << "]";
        return true;
    }

    if (ctx->raw_video().size() == 0) {
        LOG(ERROR) << "Empty video content"
                   << " [ID:" << ctx->traceid() << "]";
        return false;
    }
    resp_.Clear();
    ann_resp_.Clear();
    ann_blacklist_resp_.Clear();
    lowq_resp_.Clear();
    kakashi_resp_.Clear();

    // 1. Get feature embedding
    GeneralFeatureRequest req;
    req.set_op("query");
    req.set_url(ctx->normalization_msg().data().video(0).content());
    req.set_post_id(ctx->traceid());
    req.set_id(ctx->traceid());
    GeneralFeatureResponse pb;

    if (false == call_http_service(ctx, channel_manager_, url_, req, resp_)) {
        return false;
    }

    // 2.0 Call each service.
    skip_ = false;
    std::shared_ptr<brpc::Controller> kakashi_cntl, ann_cntl, lowq_cntl;

    if (kakashi_url_.size() != 0) {
        *req.mutable_embedding() = {resp_.embedding().begin(), resp_.embedding().end()};
        CHECK_EQ(resp_.embedding_size() % 64, 0);
        req.set_dim(64);
        req.set_op("query_write");
        kakashi_cntl = call_http_service_async(ctx, channel_manager_, kakashi_url_, req, kakashi_resp_);
    }

    if (ann_url_.size() != 0) {
        req.set_op("query_from_blacklist");
        ann_cntl = call_http_service_async(ctx, channel_manager_, ann_url_, req, ann_resp_);
    }

    if (lowq_url_.size()) {
        req.set_op("low_quality_PPT");
        lowq_cntl = call_http_service_async(ctx, channel_manager_, lowq_url_, req, lowq_resp_);
    }

    if (kakashi_cntl)
        brpc::Join(kakashi_cntl->call_id());
    if (ann_cntl)
        brpc::Join(ann_cntl->call_id());
    if (lowq_cntl)
        brpc::Join(lowq_cntl->call_id());

    bool flag = true;
    if (kakashi_cntl && kakashi_cntl->Failed()) {
        LOG(ERROR) << "Failed to access kakashi service: " << kakashi_url_ << ". " << kakashi_cntl->ErrorText()
                   << kakashi_cntl->http_response().status_code() << " [ID:" << ctx->traceid() << "]";
        flag = false;
    }
    if (lowq_cntl && lowq_cntl->Failed()) {
        LOG(ERROR) << "Failed to access lowq service: " << lowq_url_ << ". " << lowq_cntl->ErrorText()
                   << lowq_cntl->http_response().status_code() << " [ID:" << ctx->traceid() << "]";
        flag = false;
    }
    if (ann_cntl && ann_cntl->Failed()) {
        LOG(ERROR) << "Failed to access ann service: " << ann_url_ << ". " << ann_cntl->ErrorText()
                   << ann_cntl->http_response().status_code() << " [ID:" << ctx->traceid() << "]";
        flag = false;
    }

    if (ann_cntl) {
        bool call_blacklist = false;
        if (ann_resp_.results_size() != 1) {
            std::string debug;
            json2pb::ProtoMessageToJson(ann_resp_, &debug);
            LOG(ERROR) << "Failed to get ann response. " << debug;
            flag = false;
        } else {
            auto it = ann_resp_.results().begin();
            if (it->second.distance() != 0 || it->second.result() != ctx->traceid()) {
                call_blacklist = true;
            }
        }
        if (call_blacklist) {
            req.set_video(ctx->raw_video());
            req.set_op("query");
            if (false == call_http_service(ctx, channel_manager_, ann_blacklist_url_, req, ann_blacklist_resp_)) {
                LOG(ERROR) << "Failed to call blacklist. [ID:" << ctx->traceid() << "]";
                flag = false;
            }
        }
    }

    if (flag == false) {
        add_err_num();
    }

    return flag;
}

bool LikeGeneralFeatureModule::post_process(ContextPtr &ctx) {
    if (skip_)
        return true;

    int hit_flag = false;
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    std::string debug;


    if (ann_url_.size()) {
        std::string feature_id1 = ctx->traceid(), feature_id2 = "";
        (*video->mutable_detailmlresult())["general_ann_caught"] = MODEL_RESULT_PASS;
        for (auto it : ann_resp_.results()) {
            debug.clear();
            json2pb::ProtoMessageToJson(it.second, &debug);
            (*video->mutable_modelinfo())[it.first] = debug;
            feature_id1 = it.second.result();
        }
        for (auto it : ann_blacklist_resp_.results()) {
            debug.clear();
            json2pb::ProtoMessageToJson(it.second, &debug);
            (*video->mutable_modelinfo())[it.first] = debug;
            feature_id2 = it.second.result();
        }
        if (feature_id1 != ctx->traceid() && feature_id2 == ctx->traceid()) {
            (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
            (*video->mutable_detailmlresult())["general_ann_caught"] = MODEL_RESULT_REVIEW;
            hit_flag = true;
        }
        (*ctx->mutable_normalization_msg()->mutable_extradata())["feature_id1"] = feature_id1;
        (*ctx->mutable_normalization_msg()->mutable_extradata())["feature_id2"] = feature_id2;
    }


    // kakshi
    if (kakashi_url_.size()) {
        debug.clear();
        if (kakashi_resp_.distance() != 0 && kakashi_resp_.result().size()) {
            auto poster_id1 = GetUidByVid(ctx->traceid());
            auto poster_id2 = GetUidByVid(kakashi_resp_.result());
            if (poster_id1 == poster_id2 && kakashi_resp_.distance() < 0.5) {
                kakashi_resp_.set_result(ctx->traceid());
            }
        }
        json2pb::ProtoMessageToJson(kakashi_resp_, &debug);
        (*ctx->mutable_normalization_msg()->mutable_extradata())["general_kakashi"] = kakashi_resp_.result();
        (*video->mutable_modelinfo())["general_kakashi"] = debug;
        (*video->mutable_detailmlresult())["general_kakashi"] = MODEL_RESULT_PASS;
    }


    if (lowq_url_.size()) {
        debug.clear();
        json2pb::ProtoMessageToJson(lowq_resp_, &debug);
        (*video->mutable_modelinfo())["PPT_single"] = debug;
        (*video->mutable_detailmlresult())["PPT_single"] = MODEL_RESULT_PASS;
        if (lowq_resp_.ppt_result() > 0.99) {
            (*video->mutable_detailmlresult())["PPT_single"] = MODEL_RESULT_REVIEW;
            (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
            hit_flag = true;
        }
        (*ctx->mutable_model_score())["PPT_single"] = std::to_string(lowq_resp_.ppt_result());
    }

    if (hit_flag) {
        add_hit_num();
    }

    return true;
}

} // namespace bigoai;