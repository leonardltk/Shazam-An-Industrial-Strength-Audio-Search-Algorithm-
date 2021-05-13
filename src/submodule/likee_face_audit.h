#pragma once
#include "submodule/submodule.h"


namespace bigoai {

class LikeeFaceAuditSubModule : public SubModule {
public:
    LikeeFaceAuditSubModule();
    ~LikeeFaceAuditSubModule() {}
    std::string name() const override { return "LikeeFaceAuditSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const std::string &info);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

    bool feature_info_query(ContextPtr &ctx, const std::string &featue_id);

private:
    ChannelManager channel_manager_;
    ChannelManager query_channel_manager_;
    std::string url_;
    std::string query_url_;
    int max_retry_;
    int query_max_retry_;
    std::string storename_;
    std::map<std::string, int> whitelist_uids_;
    bool enable_review_;
    std::set<std::string> enable_countrys_ = {};
        // "AE", "BH", "KW", "OM", "QA", "SA", "DZ",  "EG", "IQ", "JO", "LB", "LY", "MA", "PS",
        // "TN", "YE", "SO", "SD", "MR", "DJ", "COM", "IR", "TR", "SO", "IL", "CY",  //中东：海湾6国+中东其他20;
        // "RU","UA","BY","KZ","TJ","KG","UZ","TM","AM","GE","AZ","LV","LT","EE"};   //俄语：14国;

    bool success_;
    std::string info_;
    std::vector<std::string> batch_info_;
};


} // namespace bigoai
