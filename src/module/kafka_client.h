#pragma once
#include <librdkafka/rdkafkacpp.h>
#include <syslog.h>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include "kafka_callback.h"
#include "module/module.h"
#include "util.pb.h"
#include "utils/block_queue.h"

#define RESET_EARLIEST "earliest"
#define RESET_LARGEST "largest"
#define RESET_LATEST "latest"
#define DEFAULT_QUEUED_MIN_MESSAGES "1000"
#define DEFAULT_QUEUED_MAX_KB "102400"

namespace bigoai {
class KafkaConsumerClient;
class KafkaConsumerModule : public Module {
public:
    bool init(const ModuleConfig &conf) override;
    bool do_context(ContextPtr &ctx) override;
    std::string name() const override { return "KafkaConsumerModule"; }
    void update_metrics(bool success, int que_size, uint64_t latency = 0) override;

protected:
    std::shared_ptr<KafkaConsumerClient> kkc_;

private:
    bool quit_ = false;
    std::vector<std::string> topics_;
    std::vector<std::string> groups_;
    std::set<std::string> enable_business_type_;
    std::set<std::string> enable_appid_;
};


class KafkaConsumerClient {
public:
    KafkaConsumerClient(const std::string &brokers, uint32_t threadCount = 30, uint32_t rollback_offset = 0);

    ~KafkaConsumerClient();

    bool initConsumer(const std::string &group, const std::vector<std::string> &topics, const std::string &offsetReset,
                      const std::string &queuedMinMessages, const std::string &queuedMaxKb);

    bool msgConsume(std::string &payload, bool &run, int64_t *pOffset, int32_t *pPartition, int64_t *pTime,
                    std::string *topic, uint32_t timeout);

    void runTask(std::string message, int64_t offset, int64_t partition);

    void startConsume();

private:
    std::unique_ptr<RdKafka::Conf> pConf;
    std::unique_ptr<RdKafka::Conf> pTConf;

    std::unique_ptr<ExampleRebalanceCb> ex_rebalanceCb;

    std::unique_ptr<RdKafka::KafkaConsumer> m_kafkaConsumer;

    std::string m_brokers;
    std::string m_topic;
    std::string m_group;
    std::string m_errstr;

    std::mutex m_mutex;

    std::condition_variable m_cv;
    std::map<int, int> m_history_partition;
    size_t m_last_partition_num = 0;
    uint32_t rollback_offset_ = 0;
};
}
