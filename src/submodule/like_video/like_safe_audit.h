#pragma once

#include <map>
#include <memory>
#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/submodule.h"
#include "util.pb.h"


namespace bigoai {
class LikeSafeAuditSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    std::vector<std::string> models_;
    std::vector<float> thresholds_;
    brpc::Channel channel_;

    std::string response_version_;
    TaggingServiceResponse resp_tag_;
    FineGrainedModelResponse resp_fine_grained_;

    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::string url_;
    int max_retry_;
    std::vector<std::string> enable_model_;
    std::string filter_reason_;
    std::map<std::string, std::map<std::string, std::vector<int> > > effect_blacklist_; // {model_name: {effectname: ids}}
    bool skip_;
};
}
