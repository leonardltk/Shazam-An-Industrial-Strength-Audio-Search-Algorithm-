#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class ImageNormalizationSubModule : public SubModule {
public:
    std::string name() const override { return "ImageNormalizationSubModule"; }
    bool init(const SubModuleConfig &) override;

    bool call_service(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &ctxs) override;

    bool post_process(ContextPtr &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx_batch) override;

    virtual bool is_allow_type(MessageType type);

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::vector<std::string> models_;
    std::string url_;
    ImageResponse resp_;
};

} // namespace bigoai
