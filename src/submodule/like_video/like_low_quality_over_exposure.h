#pragma once

#include "submodule/submodule.h"
#include "util.pb.h"

namespace bigoai {
class LikeLowQualityOverExposureSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    bool res_ = false;
    std::string url_;
    std::string enable_model_;
    int max_retry_;
    ExposureWarpResponse resp_;
    ExposureWarpResponse pb_video_;
};
}