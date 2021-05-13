#pragma once
#include "module/kafka_client.h"
#include "live_adaptor.pb.h"


namespace bigoai {
class LiveKafkaConsumerModule : public KafkaConsumerModule {
public:
    bool do_context(ContextPtr &ctx) override;
    std::string name() const override { return "LiveKafkaConsumerModule"; }
};
}