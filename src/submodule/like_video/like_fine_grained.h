#pragma once

#include "submodule/submodule.h"

namespace bigoai {
class FineGrainedSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> models_;
    std::vector<float> thresholds_;
    FineGrainedModelResponse resp_;

    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::string url_;
    int max_retry_;
    std::string enable_model_;
    std::string filter_reason_;
    std::map<std::string, std::vector<int> > effect_blacklist_;
    bool skip_;
};
}
