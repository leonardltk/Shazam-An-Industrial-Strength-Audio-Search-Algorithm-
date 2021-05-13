#pragma once
#include "submodule/submodule.h"


namespace bigoai {
class AudioCoreSubModule : public SubModule {
public:
    std::string name() const override { return "AudioCoreSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &_) override;
    bool post_process_batch(std::vector<ContextPtr> &_) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::string url_;
    std::vector<std::string> models_;
    int max_retry_;
    AudioCoreResponse resp_;
};
}
