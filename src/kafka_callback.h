#ifndef PROXY_KAFKA_CALLBACK_H
#define PROXY_KAFKA_CALLBACK_H

#include <librdkafka/rdkafkacpp.h>
#include <syslog.h>
#include "iostream"

namespace bigoai {

class ExampleRebalanceCb : public RdKafka::RebalanceCb {
public:
    void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
                      std::vector<RdKafka::TopicPartition *> &partitions) {
        syslog(LOG_ERR, "RebalanceCb: %s", RdKafka::err2str(err).c_str());
        for (unsigned int i = 0; i < partitions.size(); i++) {
            syslog(LOG_ERR, "part_list_print, topic:%s, partition:%d", partitions[i]->topic().c_str(),
                   partitions[i]->partition());

            std::cerr << partitions[i]->topic() << "[" << partitions[i]->partition() << "], " << partitions[i]->topic();
            std::cerr << "\n";
            std::cout << "offset " << partitions[i]->offset() << std::endl;
        }
        if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
            consumer->assign(partitions);
        } else if (err == RdKafka::ERR__REVOKE_PARTITIONS) {
            consumer->unassign();
        } else {
            syslog(LOG_ERR, "rebalance callback error");
        }
    }
};
}


#endif // PROXY_KAFKA_CALLBACK_H
