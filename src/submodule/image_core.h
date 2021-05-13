#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class ImageCoreSubModule : public SubModule {
public:
    ImageCoreSubModule();
    ~ImageCoreSubModule() {}
    std::string name() const override { return "ImageCoreSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> model_names_;
    ThresholdManager threshold_manager_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    bool success_;
    ImageCoreResponse resp_;
};

} // namespace bigoai
