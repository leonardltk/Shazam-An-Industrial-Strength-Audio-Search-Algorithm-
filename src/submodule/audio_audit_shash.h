#pragma once
#include "submodule/submodule.h"
#include <brpc/channel.h>


namespace bigoai {

class HelloShashSubModule : public SubModule {
public:
    HelloShashSubModule() = default;
    //~HelloShashSubModule() {}
    std::string name() const override { return "HelloShashSubModule"; }

    bool call_service(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;

    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const HelloShashResponse &resp) { return true; };
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;

    bool init(const SubModuleConfig &) override;

private:
    HelloShashResponse shash_audit_pb_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    bool success_;
    static brpc::Channel channel_;
};

} // namespace bigoai
