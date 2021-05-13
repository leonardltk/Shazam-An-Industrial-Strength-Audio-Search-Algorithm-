#pragma once
#include "submodule/submodule.h"


namespace bigoai {

class ImoDiscoverSubModule : public SubModule {
public:
    ImoDiscoverSubModule();
    ~ImoDiscoverSubModule() {}
    std::string name() const override { return "ImoDiscoverSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const ImoGroupChatMatchResponse &resp);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override { return true; }
    bool post_process_batch(std::vector<ContextPtr> &ctx) override { return true; }
    bool init(const SubModuleConfig &) override;
    void ParseDiscoverResult(const ImoDiscoverResponse &, ImoDiscoverResponse &, const int);

private:
    ChannelManager channel_manager_;
    std::string url_;
    std::string result_key_;

    ImoDiscoverResponse resps_;
    std::vector<bool> successes_;
    ImoDiscoverResponse video_resps_;
    std::vector<bool> video_successes_;
    bool dup_flag = false;
};

} // namespace bigoai
