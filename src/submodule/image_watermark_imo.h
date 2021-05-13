#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class ImageWatermarkImoSubModule : public SubModule {
public:
    ImageWatermarkImoSubModule();
    ~ImageWatermarkImoSubModule() {}
    std::string name() const override { return "ImageWatermarkImoSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> model_names_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    bool success_;
    bool multi_success_;
    std::vector<ImageWatermarkImoResponse> resp_;
};

} // namespace bigoai
