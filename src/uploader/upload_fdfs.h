#pragma once
#include <brpc/channel.h>
#include <brpc/server.h>
#include <string>
#include "channel_manager.h"
#include "module/module.h"
#include "utils/common_utils.h"

#define AUTH_EXPIRE 240
#define _500M 524288000

namespace bigoai {

class TokenManager;

class FastDfsUploadModule : public Module {
public:
    FastDfsUploadModule() {}
    bool init(const ModuleConfig&) override;
    std::string name() const override { return "FastDfsUploadModule"; }
    bool do_context(ContextPtr& ctx) override;

private:
    std::string url_;
    ChannelManager channel_manager_;
    int upload2Fdfs(const std::string& imageData, std::string& imgUrl);
    std::string bucket_;
    uint32_t expire_interval_;
    uint32_t file_expire_interval_;
    std::string secret_;
};

} // namespace bigoai
