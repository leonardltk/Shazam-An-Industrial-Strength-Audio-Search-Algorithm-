#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class ImageWatermarkSubModule : public SubModule {
public:
    ImageWatermarkSubModule();
    ~ImageWatermarkSubModule() {}
    std::string name() const override { return "ImageWatermarkSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> model_names_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    bool success_;
    ImageWatermarkResponse resp_;
};

} // namespace bigoai
