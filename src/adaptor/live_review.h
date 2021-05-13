#pragma once

#include <brpc/server.h>
#include <string>
#include "channel_manager.h"
#include "module/module.h"
#include "pika_client.h"
#include "util.pb.h"
#include "utils/common_utils.h"

namespace bigoai {

class LiveReviewModule : public Module {
public:
    bool init(const ModuleConfig&) override;
    bool do_context(ContextPtr& ctx) override;
    std::string name() const override { return "LiveReviewModule"; }
    bool call_service(ContextPtr& ctx);

private:
    static ChannelManager channel_manager_;
};

}
