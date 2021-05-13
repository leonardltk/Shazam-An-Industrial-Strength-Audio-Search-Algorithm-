#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "module/module.h"
#include "utils/block_queue.h"

namespace bigoai {

class VideoCodecModule : public Module {
public:
    bool init(const ModuleConfig&) override;
    bool do_context(ContextPtr&) override;
    std::string name() const { return "VideoCodecModule"; }
};

}
