#pragma once

#include "submodule/submodule.h"

namespace bigoai {
class LikeFaceAuditImageModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    FaceAuditResponse face_audit_pb_;
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;
    bool res_ = false;
};

}