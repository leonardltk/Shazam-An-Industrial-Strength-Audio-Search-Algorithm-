#pragma once
#include "module/kafka_client.h"
#include "live_adaptor.pb.h"
#include "uploader/review.h"


namespace bigoai {
class LikeReviewModule : public ReviewModule {
public:
    bool init(const ModuleConfig& conf);
    bool do_context(ContextPtr& ctx) override;
    std::string name() const override { return "LikeReviewModule"; }
    bool do_upload(const LikeReviewMessage& cb, const ContextPtr& ctx);

private:
    ChannelManager channel_manager_;
    int max_retry_;
    std::string appid_;
};
}