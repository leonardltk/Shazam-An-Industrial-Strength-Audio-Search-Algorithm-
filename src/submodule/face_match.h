#pragma once
// TODO is the same with face_match.cpp
#include "submodule/submodule.h"

namespace bigoai {

class FaceMatchSubModule : public SubModule {
public:
    FaceMatchSubModule();
    ~FaceMatchSubModule() {}
    std::string name() const override { return "FaceMatchSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const std::string &info);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

    bool feature_info_query(ContextPtr &ctx, const std::string &featue_id);

private:
    ChannelManager channel_manager_;
    ChannelManager query_channel_manager_;
    std::string url_;
    std::string query_url_;
    int max_retry_;
    int query_max_retry_;
    std::string storename_;

    bool success_;
    std::string info_;
    std::vector<std::string> batch_info_;
};

} // namespace bigoai
