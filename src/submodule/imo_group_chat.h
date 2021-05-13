#pragma once
#include "submodule/submodule.h"


namespace bigoai {

class ImoGroupChatSubModule : public SubModule {
public:
    ImoGroupChatSubModule();
    ~ImoGroupChatSubModule() {}
    std::string name() const override { return "ImoGroupChatSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const ImoGroupChatMatchResponse &resp);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    int max_retry_;
    std::string url_;
    std::string result_key_;

    bool success_;
    ImoGroupChatMatchResponse resp_;
    std::vector<ImoGroupChatMatchResponse> batch_resp_;
};

} // namespace bigoai
