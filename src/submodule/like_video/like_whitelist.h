#pragma once

#include "submodule/submodule.h"


namespace bigoai {
class LikeWhiteListSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;
    GeneralFeatureResponse resp_;
    bool skip_;
};
}