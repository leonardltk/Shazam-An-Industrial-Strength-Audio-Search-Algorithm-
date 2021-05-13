#pragma once
#include "submodule/submodule.h"


namespace bigoai {

class HelloFaceAuditSubModule : public SubModule {
public:
    HelloFaceAuditSubModule() = default;
    //~HelloFaceAuditSubModule() {}
    std::string name() const override { return "HelloFaceAuditSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const HelloFaceAuditResponse &resp) { return true; };
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    HelloFaceAuditResponse face_audit_pb_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    bool success_;
};

} // namespace bigoai
