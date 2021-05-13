#pragma once

#include <map>
#include <memory>
#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/submodule.h"
#include "util.pb.h"


namespace bigoai {
class LikeTagSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> models_;
    brpc::Channel channel_;
    ThresholdManager threshold_manager_;
    TaggingServiceResponse resp_;

    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;
    std::map<std::string, std::map<std::string, std::vector<int> > > effect_blacklist_; // {model_name: {effectname: ids}}
    bool skip_;
};
}
