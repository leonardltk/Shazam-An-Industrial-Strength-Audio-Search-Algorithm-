#pragma once

#include <brpc/channel.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "channel_manager.h"
#include "module.h"
#include "util.pb.h"
#include "utils/block_queue.h"
namespace bigoai {
class DecodeClusterDownloaderModule : public Module {
public:
    bool init(const ModuleConfig&) override;
    std::string name() const override { return "DecodeClusterDownloaderModule"; }
    bool do_context(ContextPtr& ptr) override;
    bool call_decode_cluster(ContextPtr& ctx);

private:
    std::string tcinfo_str_;
    int max_retry_;
    char host_name_[128];
    ChannelManager channel_manager_;
};
}
