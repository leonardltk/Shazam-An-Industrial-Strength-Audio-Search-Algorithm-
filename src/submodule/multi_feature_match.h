#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class MultiFeatureMatchSubModule : public SubModule {
public:
    MultiFeatureMatchSubModule(){};
    ~MultiFeatureMatchSubModule() {}
    std::string name() const override { return "MultiFeatureMatchSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

    // bool feature_info_query(ContextPtr &ctx, const std::string &feature_id);
    bool feature_info_query(ContextPtr &ctx, const std::string &feature_id, std::string& info);

private:
    ChannelManager channel_manager_;
    ChannelManager query_channel_manager_;
    std::string url_;
    std::string query_url_;
    int max_retry_;
    int query_max_retry_;
    std::string storename_;

    std::vector<bool> successes_;
    // std::string info_;
    std::vector<std::string> infos_;
    std::vector<std::string> batch_info_;
};

} // namespace bigoai
