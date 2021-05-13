#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class ImoVideoCoreSubModule : public SubModule {
public:
    ImoVideoCoreSubModule();
    ~ImoVideoCoreSubModule() {}
    std::string name() const override { return "ImoVideoCoreSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, ImoVideoCoreResponse &resp);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> model_names_;
    ThresholdManager threshold_manager_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    ImoVideoCoreResponse resp_;
    std::vector<ImoVideoCoreResponse> batch_resp_;
};

} // namespace bigoai
