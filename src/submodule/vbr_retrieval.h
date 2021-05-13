#pragma once
#include "submodule/submodule.h"


namespace bigoai {

class VbrRetrievalSubModule : public SubModule {
public:
    VbrRetrievalSubModule() = default;
    //~VbrRetrievalSubModule() {}
    std::string name() const override { return "VbrRetrievalSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const FeatureMatchResponse &resp) { return true; };
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;
    std::string key_;
    std::string result_key_;

    bool success_;
};

} // namespace bigoai
