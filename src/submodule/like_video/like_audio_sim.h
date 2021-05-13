#pragma once

#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/submodule.h"
#include "util.pb.h"


namespace bigoai {
class LikeAudioSimSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    LikeAudioSimWrap resp_;
    std::string url_;
    std::set<std::string> enable_country_;
    int max_retry_;
    bool skip_;
};
}