#pragma once
#include <brpc/channel.h>
#include <string>
#include "channel_manager.h"
#include "hive.pb.h"
#include "module/module.h"
#include "normalization_message.pb.h"

namespace bigoai {

class UploadHiveModule : public Module {
public:
    bool init(const ModuleConfig &) override;
    std::string name() const override { return "UploadHiveModule"; }
    bool do_context(ContextPtr &ctx) override;

    bool call_service(ContextPtr &ctx);

private:
    void normalization_msg_to_hive_message(const NormaliztionAuditMessage &pb, StdAuditResultHive &ret);
    bool extra_info_flag_ = false;
    std::map<std::string, std::string> extra_info_;
    static ChannelManager channel_manager_;
    std::string url_;
    std::string rewrite_business_type_ = "";
};

} // namespace bigoai
