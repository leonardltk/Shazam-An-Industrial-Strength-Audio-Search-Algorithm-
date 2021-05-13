#include "module/kafka_client.h"
#include <brpc/log.h>
#include <json2pb/json_to_pb.h>
#include "kakashi.pb.h"
#include "utils/common_utils.h"


namespace bigoai {

RegisterClass(Module, KafkaConsumerModule);

DEFINE_int32(kafka_session_timeout_ms, 8000, "");


bool KafkaConsumerModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());

    if (!conf.has_kafka_consumer()) {
        return false;
    }

    topics_.insert(topics_.end(), conf.kafka_consumer().topic().begin(), conf.kafka_consumer().topic().end());
    groups_.insert(groups_.end(), conf.kafka_consumer().groupid().begin(), conf.kafka_consumer().groupid().end());
    enable_appid_.insert(conf.kafka_consumer().enable_appid().begin(), conf.kafka_consumer().enable_appid().end());
    enable_business_type_.insert(conf.kafka_consumer().enable_business_type().begin(),
                                 conf.kafka_consumer().enable_business_type().end());


    kkc_.reset(new KafkaConsumerClient(conf.kafka_consumer().brokers(), conf.work_thread_num(),
                                       conf.kafka_consumer().rollback_offset()));
    std::string kafka_reset = RESET_LATEST;
    if (conf.kafka_consumer().reset_earliest()) {
        kafka_reset = RESET_EARLIEST;
    }
    if (false ==
        kkc_->initConsumer(groups_[0], topics_, kafka_reset, DEFAULT_QUEUED_MIN_MESSAGES, DEFAULT_QUEUED_MAX_KB))
        return false;

    LOG(INFO) << "Init " << name();
    return true;
}

bool KafkaConsumerModule::do_context(ContextPtr &ctx) {
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
    if (true != json2pb::JsonToProtoMessage(msg, ctx->mutable_normalization_msg())) {
        *metric_err_ << 1;
        LOG(ERROR) << "Failed to parse message: " << msg;
        discard_context_without_log(ctx);
        return false;
    }

    std::string appid = ctx->normalization_msg().appid();
    std::string business_type = ctx->normalization_msg().businesstype();
    if (enable_appid_.size() && enable_appid_.count(appid) == 0) {
        LOG(INFO) << "AppID filter discard message: " << msg;
        discard_context_without_log(ctx);
        return true;
    }
    if (enable_business_type_.size() && enable_business_type_.count(business_type) == 0) {
        LOG(INFO) << "BusinessType filter discard message: " << msg;
        discard_context_without_log(ctx);
        return true;
    }

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

void KafkaConsumerModule::update_metrics(bool success, int que_size, uint64_t latency) {
    *metric_cnt_ << 1;
    *metric_que_size_ << que_size;
    // Update latency in do_context
    if (!success) {
        *metric_err_ << 1;
    }
}

KafkaConsumerClient::KafkaConsumerClient(const std::string &brokers, uint32_t threadCount, uint32_t rollback_offset) {
    m_brokers = brokers;
    pConf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    pTConf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
    // std::cout << "set config " << m_brokers << std::endl;
    pConf->set("bootstrap.servers", m_brokers, m_errstr);
    rollback_offset_ = rollback_offset;

    ex_rebalanceCb.reset(new ExampleRebalanceCb());
}


KafkaConsumerClient::~KafkaConsumerClient() {
    if (m_kafkaConsumer) {
        m_kafkaConsumer->unsubscribe();
    }
}

bool KafkaConsumerClient::initConsumer(const std::string &group, const std::vector<std::string> &topics,
                                       const std::string &offsetReset, const std::string &queuedMinMessages,
                                       const std::string &queuedMaxKb) {
    if (!offsetReset.empty()) {
        if (pTConf->set("auto.offset.reset", offsetReset, m_errstr) != RdKafka::Conf::CONF_OK) {
            LOG(ERROR) << "set auto.offset.reset err: ", m_errstr.c_str();
            return false;
        }

        if (pConf->set("default_topic_conf", pTConf.get(), m_errstr) != RdKafka::Conf::CONF_OK) {
            LOG(ERROR) << "set default_topic_conf err: " << m_errstr.c_str();
            return false;
        }
    }

    if (pConf->set("group.id", group, m_errstr) != RdKafka::Conf::CONF_OK)
    // if (pConf->set("group.id", "yxp" + std::to_string(get_time_ms()), m_errstr) != RdKafka::Conf::CONF_OK)
    {
        LOG(ERROR) << "set group conf err: " << m_errstr.c_str();
        return false;
    }

    if (pConf->set("fetch.wait.max.ms", "10", m_errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR) << "set fetch.wait.max.ms conf err: " << m_errstr.c_str();
        return false;
    }

    if (pConf->set("fetch.error.backoff.ms", "5", m_errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR) << "set fetch.error.backoff.ms conf err: " << m_errstr.c_str();
        return false;
    }

    if (pConf->set("queued.min.messages", queuedMinMessages, m_errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR) << "set queued.min.messages conf err: " << m_errstr.c_str();
        return false;
    }

    if (pConf->set("queued.max.messages.kbytes", queuedMaxKb, m_errstr) != RdKafka::Conf::CONF_OK) {
        LOG(ERROR) << "set queued.max.messages.kbytes conf err: " << m_errstr.c_str();
        return false;
    }

    if (pConf->set("session.timeout.ms", std::to_string(FLAGS_kafka_session_timeout_ms), m_errstr) !=
        RdKafka::Conf::CONF_OK) {
        LOG(ERROR) << "set session.timeout.ms conf err: " << m_errstr.c_str();
        return false;
    }


    pConf->set("rebalance_cb", ex_rebalanceCb.get(), m_errstr);


    if (m_kafkaConsumer != NULL) {
        m_kafkaConsumer->close();
        m_kafkaConsumer.reset(nullptr);
    }

    m_kafkaConsumer.reset(RdKafka::KafkaConsumer::create(pConf.get(), m_errstr));
    if (m_kafkaConsumer == NULL) {
        LOG(ERROR) << "Failed to create kafkaconsumer";
        return false;
    }

    RdKafka::ErrorCode err = m_kafkaConsumer->subscribe(topics);
    for (size_t i = 0; i < topics.size(); ++i) {
        LOG(INFO) << "Subscribe topic: " << topics[i];
    }

    if (err) {
        LOG(ERROR) << "Failed to subscribe to " << topics.size()
                   << " topics, errMsg: " << RdKafka::err2str(err).c_str();
        return false;
    }

    LOG(INFO) << "Created kafkaconsumer " << m_kafkaConsumer->name().c_str();
    return true;
}


bool KafkaConsumerClient::msgConsume(std::string &payload, bool &run, int64_t *pOffset, int32_t *pPartition,
                                     int64_t *pTime, std::string *topic, uint32_t timeout) {
    run = true;
    std::unique_ptr<RdKafka::Message> message;

    if (m_kafkaConsumer != NULL) {
        message.reset(m_kafkaConsumer->consume(timeout));
    } else {
        LOG(ERROR) << "consumer is not created";
        run = false;
        return false;
    }

    switch (message->err()) {
        case RdKafka::ERR__TIMED_OUT:
            // LOG(INFO) << "kafka timed out, " << m_topic << ", Maybe kafka is empty!";
            run = false;
            return false;

        case RdKafka::ERR_NO_ERROR:
            payload.assign((const char *)message->payload(), message->len());
            if (pPartition != NULL) {
                *pPartition = message->partition();
            }
            if (pOffset != NULL) {
                *pOffset = message->offset();
            }
            if (pTime != NULL) {
                *pTime = message->timestamp().timestamp;
            }
            *topic = message->topic_name();
            break;
        case RdKafka::ERR__PARTITION_EOF:
            run = false;
            return false;

        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            LOG(ERROR) << "Consume failed: " << message->errstr().c_str();
            run = false;
            return false;

        default:
            LOG(ERROR) << "Consume failed: " << message->errstr().c_str();
            run = false;
            return false;
    }

    std::vector<RdKafka::TopicPartition *> partitions;
    m_kafkaConsumer->assignment(partitions);
    if (m_last_partition_num == 0u) {
        m_last_partition_num = partitions.size();
    }

    // Maybe rebalance again. we shoule reset m_history_partition
    if (m_last_partition_num != partitions.size()) {
        std::set<int> ids;
        std::set<int> remove_ids;
        for (size_t i = 0; i < partitions.size(); ++i) {
            ids.insert(partitions[i]->partition());
        }
        for (auto it : m_history_partition) {
            if (ids.find(it.first) == ids.end()) {
                remove_ids.insert(it.first);
            }
        }
        for (auto it : remove_ids) {
            LOG(INFO) << "Kafka remove history parition: " << it;
            m_history_partition.erase(it);
        }
        m_last_partition_num = partitions.size();
    }

    for (size_t i = 0; i < partitions.size(); ++i) {
        if (partitions[i])
            delete partitions[i];
    }

    if (rollback_offset_ != 0 && m_history_partition.find(message->partition()) == m_history_partition.end()) {
        LOG(INFO) << "rollback offset... [Partition: " << message->partition() << "]";
        m_history_partition[message->partition()] = 1;
        std::vector<RdKafka::TopicPartition *> partitions;
        m_kafkaConsumer->assignment(partitions);
        m_kafkaConsumer->position(partitions);
        for (size_t i = 0; i < partitions.size(); ++i) {
            if (partitions[i]->partition() != message->partition())
                continue;
            int new_offset = std::max(int64_t(1), partitions[i]->offset() - rollback_offset_);
            LOG(INFO) << "Reset partition " << message->partition() << ", old offset: " << partitions[i]->offset()
                      << ", back to: " << new_offset;
            partitions[i]->set_offset(new_offset);
            break;
        }
        m_kafkaConsumer->assign(partitions);
        for (size_t i = 0; i < partitions.size(); ++i) {
            if (partitions[i])
                delete partitions[i];
        }
    }

    return true;
}


void KafkaConsumerClient::startConsume() {
    std::string msg;
    while (true) {
        bool run;

        int64_t offset;
        int32_t partition;
        std::string topic;

        this->msgConsume(msg, run, &offset, &partition, nullptr, &topic, 1000);
    }
}
}
