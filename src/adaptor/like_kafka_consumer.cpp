#include "adaptor/like_kafka_consumer.h"
#include <brpc/log.h>
#include <json2pb/json_to_pb.h>
#include "utils/common_utils.h"
#include "live_adaptor.pb.h"

using namespace bigoai;


RegisterClass(Module, LikeKafkaConsumerModule);

float eps = 0.0001;

float sqrt(float a) {
    float end = a > 1 ? a : 1;
    float beg = a > 1 ? 1 : a;
    float mid = (end + beg) / 2;
    while (true) {
        float diff = mid * mid - a;
        if (std::abs(diff) < eps) {
            break;
        }
        if (diff > 0) {
            mid = (mid + beg) / 2;
        } else {
            mid = (mid + end) / 2;
        }
    }
    return mid;
}


bool LikeKafkaConsumerModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());

    if (!conf.has_kafka_consumer()) {
        return false;
    }

    std::vector<std::string> topics, groups;
    topics.insert(topics.end(), conf.kafka_consumer().topic().begin(), conf.kafka_consumer().topic().end());
    groups.insert(groups.end(), conf.kafka_consumer().groupid().begin(), conf.kafka_consumer().groupid().end());

    kkc_.reset(new KafkaConsumerClient(conf.kafka_consumer().brokers(), conf.work_thread_num(),
                                       conf.kafka_consumer().rollback_offset()));
    std::string kafka_reset = RESET_LATEST;
    if (conf.kafka_consumer().reset_earliest()) {
        kafka_reset = RESET_EARLIEST;
    }
    if (false == kkc_->initConsumer(groups[0], topics, kafka_reset, DEFAULT_QUEUED_MIN_MESSAGES, DEFAULT_QUEUED_MAX_KB))
        return false;

    callback_url_ = conf.kafka_consumer().callback_url();
    LOG(INFO) << "Init " << name();
    return true;
}

bool LikeKafkaConsumerModule::do_context(ContextPtr &ctx) {
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
    LikeMessage like_msg;
    if (true != json2pb::JsonToProtoMessage(msg, &like_msg)) {
        *metric_err_ << 1;
        LOG(ERROR) << "Failed to parse message: " << msg;
        discard_context(ctx);
        return false;
    }

    LikeMessage::ExtInfo ext_info;
    if (true != json2pb::JsonToProtoMessage(like_msg.ext_info(), &ext_info)) {
        LOG(ERROR) << "Failed to parse extinfo: " << like_msg.ext_info();
        discard_context(ctx);
        return false;
    }
    LikeMessage::AttachInfo attch_info;
    if (true != json2pb::JsonToProtoMessage(like_msg.attach_info(), &attch_info)) {
        LOG(ERROR) << "Failed to parse attachinfo: " << like_msg.attach_info();
        discard_context(ctx);
        return false;
    } else {
        if (attch_info.cutme_id().size() != 0 && attch_info.cutme_url().size() != 0) {
            try {
                // stoi could throw exception
                auto cutme_id = std::stoi(attch_info.cutme_id());
                auto cutme_url = attch_info.cutme_url();
                ctx->set_cutme_id(cutme_id);
                ctx->set_cutme_url(cutme_url);
                VLOG(10) << "CutMe video, cutmeid: " << attch_info.cutme_id() << ". [ID:" << ctx->traceid() << "]";
            } catch (...) {
                LOG(ERROR) << "Failed to get cutme id info: " << attch_info.cutme_id() << ". [ID:" << ctx->traceid()
                           << "]";
            }
        }
        ctx->set_video_src(attch_info.training_model_info().src());
        ctx->set_is_duet(false);
        // Try to parse effect data field.
        LikeMessage::EffectData effect;
        LikeMessage::EffectData::OtherValue other_value;
        ctx->set_effect_data(attch_info.effect_data());
        if (true != json2pb::JsonToProtoMessage(attch_info.effect_data(), &effect)) {
            LOG(ERROR) << "Failed to parse effect data: " << like_msg.attach_info();
        } else {
            if (true != json2pb::JsonToProtoMessage(effect.other_value(), &other_value)) {
                LOG(ERROR) << "Failed to parse other value: " << like_msg.attach_info();
            }
        }
        if (other_value.duet_video_id().size() && other_value.duet_video_id() != "0") {
            ctx->set_is_duet(true);
        }
    }

    auto norm_msg = ctx->mutable_normalization_msg();
    norm_msg->set_orderid(like_msg.postid());
    norm_msg->set_uid(like_msg.uid());
    norm_msg->set_appid(like_msg.appid());
    norm_msg->set_country(like_msg.country());
    norm_msg->set_reporttime((uint64_t)ext_info.post_time() * 1000); // second -> ms
    norm_msg->set_callbackurl(callback_url_);

    (*norm_msg->mutable_extradata())["extInfo"] = like_msg.ext_info();
    (*norm_msg->mutable_extradata())["attachInfo"] = like_msg.attach_info();

    AuditDataSubType cover_data;
    cover_data.set_content(ext_info.image1());
    cover_data.set_sid(like_msg.seqid());
    cover_data.set_id("0");
    *norm_msg->mutable_data()->add_pic() = cover_data;

    AuditDataSubType like_data;
    like_data.set_content(like_msg.video_url());
    (*like_data.mutable_subextradata())["duration"] = std::to_string(attch_info.training_model_info().post_duration());
    like_data.set_sid(like_msg.seqid());
    like_data.set_id("0");
    *norm_msg->mutable_data()->add_video() = like_data;

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