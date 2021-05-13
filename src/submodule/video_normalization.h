#pragma once
#include "submodule/submodule.h"
#include "threshold_manager.h"

namespace bigoai {

class VideoNormalizationSubModule : public SubModule {
public:
    std::string name() const override { return "VideoNormalizationSubModule"; }

    bool call_service_batch(std::vector<ContextPtr> &ctxs) override;
    bool post_process_batch(std::vector<ContextPtr> &ctxs) override;

    bool call_service(ContextPtr &ctx) override;
    bool call_service(ContextPtr &ctx, VideoResponse &resp);

    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, VideoResponse &resp);

    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::vector<std::string> models_;
    std::string url_;
    std::vector<VideoResponse> resps_;
};

} // namespace bigoai
