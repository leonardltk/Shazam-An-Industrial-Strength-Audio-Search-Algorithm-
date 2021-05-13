#include "adaptor/live_kafka_consumer.h"
#include <brpc/log.h>
#include <json2pb/json_to_pb.h>
#include "utils/common_utils.h"
#include "live_adaptor.pb.h"

using namespace bigoai;

bool LiveKafkaConsumerModule::do_context(ContextPtr &ctx) {
    std::string msg;
    bool run;
    int64_t offset;
    int32_t partition;
    int64_t timestamp;
    std::string topic;
    if (true != kkc_->msgConsume(msg, run, &offset, &partition, &timestamp, &topic, 10000)) {
        discard_context(ctx);
        return false;
    }
    int64_t start_time = get_time_ms();

    // KafkaConsumer pb;
    LiveMessage live_msg;
    if (true != json2pb::JsonToProtoMessage(msg, &live_msg)) {
        *metric_err_ << 1;
        LOG(ERROR) << "Failed to parse message: " << msg;
        discard_context(ctx);
        return false;
    }

    auto norm_msg = ctx->mutable_normalization_msg();
    norm_msg->set_orderid(std::to_string(live_msg.seq_id()));
    norm_msg->set_uid(live_msg.uid());
    norm_msg->set_appid(live_msg.appid());
    norm_msg->set_businesstype("LiveAdaptor");
    norm_msg->set_country(live_msg.country());
    norm_msg->set_reporttime(std::stoul(live_msg.report_time()));
    (*norm_msg->mutable_extradata())["extPar"] = live_msg.extpar();
    (*norm_msg->mutable_extradata())["attachInfo"] = live_msg.attachinfo();
    (*norm_msg->mutable_extradata())["serial"] = live_msg.serial();
    (*norm_msg->mutable_extradata())["type"] = live_msg.type();
    (*norm_msg->mutable_extradata())["business"] = live_msg.business();
    // norm_msg->set_callbackurl("");

    AuditDataSubType live_data;
    live_data.set_content(live_msg.url());
    live_data.set_sid(live_msg.serial());
    live_data.set_id("0");

    *norm_msg->mutable_data()->add_live() = live_data;

    ctx->set_start_time(get_time_ms());
    ctx->set_traceid(ctx->normalization_msg().orderid());

    std::string date;
    unixTime2Str(timestamp / 1000, date);
    LOG(INFO) << "Get msg from kafka. partition:" << partition << ", offset: " << offset << ", topic: " << topic
              << ", kafka timestamp: " << timestamp << "(" << date << ")"
              << "[ID:" << ctx->traceid() << "]";
    (*ctx->mutable_normalization_msg()->mutable_timestamp())["read_kafka_time"] = get_time_ms();
    (*ctx->mutable_normalization_msg()->mutable_timestamp())["write_kafka_time"] = timestamp;

    int64_t end_time = get_time_ms();
    if (timestamp > 0 && end_time >= timestamp) {
        *metric_latency_ << (end_time - timestamp);
    } else {
        *metric_latency_ << (end_time - start_time);
    }
    return true;
}