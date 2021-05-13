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
class DurationFilterModule : public Module {
public:
    bool init(const ModuleConfig&) override;
    bool do_context(ContextPtr& in_buf) override;

private:
    int32_t max_second_;
    int32_t min_second_;
};
}
