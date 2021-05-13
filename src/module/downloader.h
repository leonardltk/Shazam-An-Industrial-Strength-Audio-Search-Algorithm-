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
class DownloaderModule : public Module {
public:
    bool init(const ModuleConfig &) override;
    bool do_context(ContextPtr &in_buf) override;

    bool download_image(ContextPtr &ctx);
    bool download_video(ContextPtr &ctx);
    bool download_voice(ContextPtr &ctx);

    bool redirect_retry(std::shared_ptr<brpc::Channel> channel, const std::string &url, int redirect_count, std::string &content);

private:
    std::shared_ptr<brpc::Channel> get_channel(std::string url);
    ChannelManager channel_manager_;
    bool enable_image_;
    bool enable_video_;
    bool enable_voice_;
};
}
