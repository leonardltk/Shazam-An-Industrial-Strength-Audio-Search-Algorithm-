#pragma once

#include "submodule/submodule.h"
#include "util.pb.h"

namespace bigoai {
class LikeLowQualityExposureSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    bool res_ = false;
    std::string url_;
    ExposureWarpResponse pb_img_;
    ExposureWarpResponse pb_video_;
    std::string enable_model_;
    bool enable_cover_;
    int max_retry_;
};
}