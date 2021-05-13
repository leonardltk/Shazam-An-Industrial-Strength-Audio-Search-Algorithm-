#include <sstream>
#include <brpc/callback.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include "module/decode_cluster_downloader.h"
#include "utils/base64.h"
#include "utils/common_utils.h"
#include "util.pb.h"

namespace bigoai {

bool DecodeClusterDownloaderModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    if (!conf.has_downloader()) {
        return false;
    }
    work_thread_num_ = conf.work_thread_num();
    channel_manager_.set_timeout_ms(conf.downloader().timeout_ms());
    channel_manager_.set_lb(conf.downloader().lb());
    channel_manager_.set_retry_time(0);
    max_retry_ = conf.downloader().max_retry();
    tcinfo_str_ = "eyJ0eXBlIjogMCwgImFwcGlkIjogMTEsICJ2ZXJzaW9uIjogMCwgImRjRmxhZyI6IDB9";
    gethostname(host_name_, 128);
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool DecodeClusterDownloaderModule::call_decode_cluster(ContextPtr &ctx) {
    std::string video_url = ctx->normalization_msg().data().video(0).content();
    // Set header
    DecodeClusterExtInfo extinfo;
    std::istringstream is(ctx->traceid());
    int64_t reqid;
    is >> reqid;

    extinfo.set_reqid(reqid);
    extinfo.set_reqtime(time(NULL));
    extinfo.set_msgfrom("kafka-consumer-pre-request");
    extinfo.set_postid(ctx->traceid());
    extinfo.set_sourceip(host_name_);
    std::string extinfo_json;
    json2pb::ProtoMessageToJson(extinfo, &extinfo_json);
    extinfo_json = safeBase64encode(extinfo_json);

    // Download Video
    bool video_success = false;
    for (int i = 0; i < max_retry_; ++i) {
        brpc::Controller cntl;
        cntl.http_request().uri().SetHttpURL(video_url);
        cntl.http_request().SetHeader("extinfo", extinfo_json);
        cntl.http_request().uri().SetQuery("tcinfo", tcinfo_str_);
        auto channel = channel_manager_.get_channel(video_url);
        if (channel.get() == nullptr) {
            LOG(ERROR) << "Failed to get channel: " << video_url;
            continue;
        }

        channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
        if (cntl.Failed()) {
            LOG(ERROR) << "Failed to download video from decode cluster. url: " << video_url
                       << ", err: " << cntl.ErrorText() << ", http status: " << cntl.http_response().status_code()
                       << "[ID:" << ctx->traceid() << "]";
            continue;
        }
        if (cntl.http_response().GetHeader("X-Tc-Tid") == NULL) {
            LOG(INFO) << "Failed to call decode cluster. we get a raw video. ip: " << cntl.remote_side()
                      << "[ID:" << ctx->traceid() << "]";
            continue;
        }

        video_success = true;

        std::string resp_str = cntl.response_attachment().to_string();
        char *cursor = (char *)resp_str.data();
        uint32_t len = ntohl(*reinterpret_cast<uint32_t *>(cursor));
        if (len + 4 > resp_str.size()) {
            LOG(ERROR) << "Invailed len";
            continue;
        }
        std::string meta_info = resp_str.substr(4, len);
        meta_info = Base64Decode(meta_info);
        DecodeClusterMetaInfo metaInfo;
        json2pb::JsonToProtoMessage(meta_info, &metaInfo);
        *ctx->mutable_decode_info() = metaInfo;
        std::string meta_string;
        json2pb::ProtoMessageToJson(ctx->decode_info(), &meta_string);
        VLOG(10) << "decode cluster: " << meta_string << "[" << ctx->traceid() << "]";
        break;
    }

    if (video_success == false) {
        LOG(INFO) << "Failed to call decode cluster " << video_url << " [ID:" << ctx->traceid() << "]";
    }
    return video_success;
}

bool DecodeClusterDownloaderModule::do_context(ContextPtr &ctx) {
    auto ret = call_decode_cluster(ctx);
    set_context_timestamp(ctx, "after_decode_cluster");
    return ret;
}

RegisterClass(Module, DecodeClusterDownloaderModule);

} // namespace bigoai