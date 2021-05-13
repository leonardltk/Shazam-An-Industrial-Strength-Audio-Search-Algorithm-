#include "module/video/video_codec.h"
#include <brpc/callback.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <thread>
#include "decode_video/ffmpeg.h"
#include "proxy.pb.h"
#include "utils/common_utils.h"


namespace bigoai {

bool get_video_duration(std::string video, float &duration) {
    FFMpegVideoDecoder decoder;
    decoder.Init(0, 0, 0, 0);
    int ret = decoder.GetVideoDuration((uint8_t *)video.data(), video.size(), &duration);
    if (ret != 0) {
        return false;
    }
    return true;
}

bool VideoCodecModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    work_thread_num_ = conf.work_thread_num();
    LOG(INFO) << "Init " << name();
    return true;
}

bool VideoCodecModule::do_context(ContextPtr &ctx) {
    std::string raw_video = ctx->raw_video();

    if (raw_video.size() == 0) {
        LOG(ERROR) << "Empty raw video. [ID:" << ctx->traceid() << "]";
        return true;
    }

    float duration = 0;
    if (true != get_video_duration(raw_video, duration)) {
        CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
        LOG(ERROR) << "Failed to parse video info : " << ctx->normalization_msg().data().video(1).content()
                   << ", video size: " << ctx->raw_video().size() << "[ID:" << ctx->traceid() << "]";
        ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->set_failed_module(name());
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_subextradata())["duration"] = "0";
        return false;
    }
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_subextradata())["duration"] =
        std::to_string(duration);
    return true;
}

RegisterClass(Module, VideoCodecModule);
} // namespace bigoai
