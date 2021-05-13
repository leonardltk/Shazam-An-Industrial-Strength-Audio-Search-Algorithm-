#pragma once

#include "submodule/submodule.h"


namespace bigoai {
class LikeWatermarkSubModule : public SubModule {
public:
    std::string name() const override { return "LikeWatermarkSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    std::set<std::string> target_clsnames = {"tiktok"};
    std::string url_;
    std::string name_;
    WatermarkWrap::WatermarkInfo watermark_info_;
    const float thd1_ = 0.92, thd2_ = 0.9;
    int max_retry_;
    bool skip_;
};
}