#include "module/audio/hello_audio_codec.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <atomic>
#include <thread>
#include "decode_audio/decode_audio.h"
#include "proxy.pb.h"
#include "utils/common_utils.h"

namespace bigoai {

int HelloAudioCodecModule::decode_audio_impl(std::string &raw_audio, std::string &ret_pcm) {
    int error_code = RET_OK;
    int pcm_len = 0;
    ret_pcm.resize(out_sample_rate_ * 2 * 600); // 10 min buffer.
    try {
        pcm_len = audioUnpackDecode(out_sample_rate_, (uint8_t *)raw_audio.c_str(), raw_audio.size(),
                                    (uint8_t *)ret_pcm.data(), ret_pcm.size(), &error_code);
    } catch (...) {
        LOG(ERROR) << "Unkown exception captured.";
        pcm_len = 0;
        error_code = RET_DECODE_ERROR;
    }
    if (error_code != RET_OK) {
        LOG(ERROR) << "Failed to decode audio. error code: " << error_code << ", pcm len: " << pcm_len;
        return -1;
    }
    ret_pcm.resize(pcm_len);
    return 0;
}

// int HelloAudioCodecModule::decode_audio_impl(std::string &raw_audio, std::string &ret_pcm) {
//     try {
//         yymobile::AudioFlow *audioFlowInstance = new yymobile::AudioFlow();
//         //第一个参数MP3=0,M4A=1,PCM=2,AAC=3,OPUS=4,SILK=5,WAV=6,G729=7,Bigolive=100,Bles=101,Likee=102,Hello=103,Tob104
//         audioFlowInstance->SetInput(yymobile::Bles);
//         audioFlowInstance->SetOutput(yymobile::PCM, out_sample_rate_, 1, 1);
//         audioFlowInstance->SetProcess(true, true, true, false, false);
//         std::vector<unsigned char> outputData;
//         audioFlowInstance->flowAudioWork((uint8_t *)raw_audio.c_str(), raw_audio.size(), outputData);
//         ret_pcm = std::string(outputData.begin(), outputData.end());
//     } catch (...) {
//         LOG(ERROR) << "Unkown exception captured.";
//     }
//     return 0;
// }


bool HelloAudioCodecModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    if (!conf.has_hello_audio_codec()) {
        return false;
    }
    work_thread_num_ = conf.work_thread_num();
    out_sample_rate_ = conf.hello_audio_codec().out_sample_rate();
    enable_wav_ = conf.hello_audio_codec().enable_wav();
    enable_aac_ = conf.hello_audio_codec().enable_aac();
    if (enable_wav_ || enable_aac_) {
        LOG(WARNING) << "config field hello_audio_codec.[enable_wav|enable_aac] has been decrypted, using "
                        "util/codec_util.h/get_wav_audio plz.";
    }
    encode_sidecar_url_ = conf.hello_audio_codec().encode_sidecar_url();
    LOG(INFO) << "Init " << name();
    return true;
}

bool HelloAudioCodecModule::do_context(ContextPtr &ctx) {
    std::string raw_audio = ctx->raw_audio();
    if (raw_audio.size() == 0) {
        LOG(ERROR) << "Empty raw audio. [ID:" << ctx->traceid() << "]";
        return false;
    }
    std::string pcm;

    if (0 != decode_audio_impl(raw_audio, pcm)) {
        CHECK_EQ(ctx->normalization_msg().data().voice_size(), 1);
        LOG(ERROR) << "Failed to decode audio: " << ctx->url_audio() << ", audio size: " << ctx->raw_audio().size()
                   << "[ID:" << ctx->traceid() << "]";
        ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->set_failed_module(name());
        return false;
    }

    float duration = pcm.size() / out_sample_rate_ / 2.0;
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_subextradata())["duration"] =
        std::to_string(duration);
    VLOG(20) << "Audio size: " << raw_audio.size() << ", pcm size: " << pcm.size();
    ctx->set_pcm_audio(pcm);
    ctx->set_pcm_sample_rate(out_sample_rate_);
    (*ctx->mutable_normalization_msg()->mutable_timestamp())["after_decode"] = get_time_ms();
    VLOG(20) << "Raw audio size: " << raw_audio.size() << ", pcm size: " << pcm.size();
    return true;
}

RegisterClass(Module, HelloAudioCodecModule);
} // namespace bigoai
