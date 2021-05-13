#pragma once
#include <memory>
#include <string>
#include <vector>
#include "channel_manager.h"
#include "module/module.h"
#include "pika_client.h"
#include "util.pb.h"
#include "utils/block_queue.h"

namespace bigoai {
class DuplicatedModule : public Module {
public:
    // std::string name() const override { return "DuplicatedModule"; }
    bool init(const ModuleConfig&) override;
    bool do_context(ContextPtr& in_buf) override;

private:
    std::string op_;
    std::shared_ptr<PikaClient> pika_;
};
}
