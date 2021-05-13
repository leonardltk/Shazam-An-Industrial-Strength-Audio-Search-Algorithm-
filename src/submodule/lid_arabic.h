#pragma once
#include "submodule/submodule.h"


namespace bigoai {
class LidArabicSubModule : public SubModule {
public:
    LidArabicSubModule();
    ~LidArabicSubModule() {}
    std::string name() const override { return "LidArabicSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &_) override;
    bool post_process_batch(std::vector<ContextPtr> &_) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    AudioCoreResponse resp_;
    std::vector<std::string> models_;
    std::vector<size_t> already_sent_;
    int max_retry_;
    std::string url_;
};

}
