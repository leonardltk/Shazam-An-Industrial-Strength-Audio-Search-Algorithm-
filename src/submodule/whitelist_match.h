#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class WhitelistMatchSubModule : public SubModule {
public:
    WhitelistMatchSubModule();
    ~WhitelistMatchSubModule() {}
    std::string name() const override { return "WhitelistMatchSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const WhitelistMatchResult &info);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

    bool feature_info_query(ContextPtr &ctx, const std::string &featue_id);

private:
    ChannelManager channel_manager_;
    std::string url_;
    std::string result_key_;
    int max_retry_;

    bool success_;
    WhitelistMatchResult info_;
    std::vector<WhitelistMatchResult> batch_info_;
    bool to_process_ = false;
};

} // namespace bigoai
