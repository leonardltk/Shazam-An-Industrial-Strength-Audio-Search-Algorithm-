#pragma once
#include <brpc/channel.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "channel_manager.h"
#include "module/module.h"
#include "util.pb.h"
#include "utils/block_queue.h"

namespace bigoai {
class IspFilter;
class IspFilterModule : public Module {
public:
    bool init(const ModuleConfig &conf) override;
    bool do_context(ContextPtr &in_buf) override;
    std::string name() const override { return "IspFilterModule"; }

private:
    std::string pass_isp_;
    bool sp_logic_;
};
}
