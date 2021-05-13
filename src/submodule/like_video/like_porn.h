#pragma once

#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/submodule.h"
#include "util.pb.h"


namespace bigoai {
class LikePornSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::vector<std::string> enable_models_;
    LikeCoreResponse resp_;
    std::string url_;
    int max_retry_;
};
}