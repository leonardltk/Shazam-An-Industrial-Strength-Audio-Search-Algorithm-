#pragma once
#include <brpc/channel.h>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "module/module.h"
#include "utils/block_queue.h"

namespace bigoai {

class ImageSplitModule : public Module {
public:
    bool init(const ModuleConfig&) override;
    void start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) override;
    std::string name() const { return "ImageSplitModule"; }
};

}
