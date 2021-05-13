#include "submodule/like_video/like_low_quality_general.h"

using namespace bigoai;

bool LikeLowQualityGeneralSubModule::init(const SubModuleConfig& conf) {
    set_name(conf.instance_name());
    return true;
}


bool LikeLowQualityGeneralSubModule::call_service(ContextPtr& ctx) { return true; }

bool LikeLowQualityGeneralSubModule::post_process(ContextPtr& ctx) {
    auto non_silent = ctx->decode_info().nonsilentaudiopercent();
    auto max_audio_energy = ctx->decode_info().maxaudioenergy();
    auto avg_audio_energy = ctx->decode_info().avgaudioenergy();
    auto duration = ctx->decode_info().duration();

    std::ostringstream os;
    os << "{\"non_silent\":\"" << non_silent << "\",\"max_audio_energy\":\"" << max_audio_energy
       << "\",\"avg_audio_energy\":\"" << avg_audio_energy << "\"}";

    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);

    (*ctx->mutable_model_score())["silent"] = os.str();
    (*ctx->mutable_model_score())["play_short"] = std::to_string(duration);
    (*video->mutable_detailmlresult())["silent"] = os.str();
    (*video->mutable_detailmlresult())["play_short"] = std::to_string(duration);

    bool cond = (duration <= 1e-6 && non_silent <= 1e-6 && max_audio_energy <= 1e-6 && avg_audio_energy <= 1e-6);
    if (cond) {
        LOG(ERROR) << "decode meta info empty, duration =" << duration << "[ID:" << ctx->traceid() << "]";
    }

    bool hit_flag = false;
    (*video->mutable_detailmlresult())["silent"] = MODEL_RESULT_PASS;
    if (!cond && non_silent <= 0.01) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*video->mutable_detailmlresult())["silent"] = MODEL_RESULT_REVIEW;
        hit_flag = true;
    }

    (*video->mutable_detailmlresult())["play_short"] = MODEL_RESULT_PASS;
    if (!cond && duration < 5) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*video->mutable_detailmlresult())["play_short"] = MODEL_RESULT_REVIEW;
        hit_flag = true;
    }
    if (hit_flag) {
        add_hit_num();
    }
    return true;
}

RegisterClass(SubModule, LikeLowQualityGeneralSubModule);