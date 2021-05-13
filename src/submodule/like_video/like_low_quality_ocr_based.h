#pragma once

#include "submodule/submodule.h"


namespace bigoai {
class LikeLowQualityOcrBasedSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    bool is_success_ = false;
    OCRBasedLowQualityInfo resp_;
    int max_retry_;
    std::string url_;
    std::string base_op_;
};
}