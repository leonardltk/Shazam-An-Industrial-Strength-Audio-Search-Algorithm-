#pragma once
#include <set>
#include "submodule/submodule.h"
#include "threshold_manager.h"

namespace bigoai {

class CrawlerVideoSubModule : public SubModule {
public:
    std::string name() const override { return "CrawlerVideoSubModule"; }

    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;

    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::vector<std::string> models_;
    std::string url_;
    VideoResponse resp_;
    const std::set<std::string> country_list_{"NG", "ZA", "BJ", "BW", "BF", "BI", "CM", "CF", "TD", "CG",
                                              "ET", "GA", "GM", "GM", "GH", "GN", "KE", "LS", "LR", "MG",
                                              "MW", "ML", "NA", "NE", "SN", "SC", "SL", "SZ", "TZ", "TG",
                                              "UG", "ZW", "ZM", "CD", "CI", "ER", "KM", "RE", "RW"};
};

} // namespace bigoai
