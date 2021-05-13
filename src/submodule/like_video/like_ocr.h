#pragma once

#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/submodule.h"
#include "util.pb.h"


namespace bigoai {
class LikeOcrSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;
    bool parse_response(const LikeOcrInfo &response, ContextPtr &ctx);
    bool call_keywords(std::string ocr_text, const ContextPtr &ctx);

private:
    ChannelManager channel_manager_;
    ChannelManager keyword_channel_manager_;
    std::string url_;
    std::string keyword_url_;
    int max_retry_;
    int keyword_max_retry_;
    LikeOcrInfo ocr_info_;
    SensitiveWordResponse sensitive_resp_;
    std::vector<SensitiveWordHitRule> hit_rules_;
};
}