#pragma once
#include "module/kafka_client.h"
#include "live_adaptor.pb.h"


namespace bigoai {
class LikeKafkaConsumerModule : public KafkaConsumerModule {
public:
    bool init(const ModuleConfig &conf);
    bool do_context(ContextPtr &ctx) override;
    std::string name() const override { return "LikeKafkaConsumerModule"; }

private:
    std::string callback_url_;
};
}