#pragma once

#include <brpc/server.h>
#include <string>
#include "channel_manager.h"
#include "module/module.h"
#include "util.pb.h"
#include "utils/common_utils.h"

namespace bigoai {

class ReviewModule : public Module {
public:
    bool init(const ModuleConfig& conf) override;
    bool do_context(ContextPtr& ctx) override;
    std::string name() const override { return "ReviewModule"; }

    bool call_service(ContextPtr& ctx);

private:
    static ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;
};
}
