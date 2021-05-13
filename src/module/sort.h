#pragma once
#include <string>
#include <vector>
#include <queue>
#include "module/module.h"
#include "util.pb.h"
#include "utils/block_queue.h"

namespace bigoai {


class LRU;
class SortModule : public Module {
public:
    bool init(const ModuleConfig &) override;
    void start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) override;

private:
    uint64_t delay_ms_;
    int sort_buffer_size_;
    std::shared_ptr<LRU> lru_;
};
}
