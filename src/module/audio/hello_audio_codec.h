#pragma once
#include <brpc/channel.h>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "module/module.h"
#include "utils/block_queue.h"

namespace bigoai {

class HelloAudioCodecModule : public Module {
public:
    bool init(const ModuleConfig &) override;
    bool do_context(ContextPtr &) override;
    std::string name() const { return "HelloAudioCodecModule"; }

private:
    int decode_audio_impl(std::string &raw_audio, std::string &ret_pcm);
    int encode_audio(const std::string &pcm, std::string &enc_data, std::string enc_type);
    uint32_t out_sample_rate_;
    bool enable_wav_;
    bool enable_aac_;
    std::string encode_sidecar_url_;
};
}
