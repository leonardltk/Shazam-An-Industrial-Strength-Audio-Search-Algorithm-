#pragma once

#include "submodule/submodule.h"


namespace bigoai {
class LikeCoverSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    LikeCoreResponse resp_;
    ChannelManager channel_manager_;
    std::string url_;
    std::string enable_model_;
    int max_retry_;
    ThresholdManager threshold_manager_;
    bool skip_;
};
}
