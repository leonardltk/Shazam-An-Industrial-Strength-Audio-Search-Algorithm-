#include <mutex>
#include <thread>
#include "module/sort.h"

namespace bigoai {

uint64_t get_ctx_timestamp(ContextPtr ctx) { return ctx->normalization_msg().reporttime(); }
struct Less {
    bool operator()(const ContextPtr &a, const ContextPtr &b) {
        return a->normalization_msg().reporttime() > b->normalization_msg().reporttime();
    }
};


class LRU {
public:
    LRU() = default;
    bool put(ContextPtr &ctx) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (storage_.size() > max_capacity_) {
            return false;
        }
        storage_.push(ctx);
        return true;
    }


    // Return oldest message.
    ContextPtr pop() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (storage_.size()) {
            ContextPtr ret = storage_.top();
            storage_.pop();
            return ret;
        }
        return nullptr;
    }

    size_t size() { return storage_.size(); }

    void set_max_capacity(size_t max_capacity) { max_capacity_ = max_capacity; }

private:
    std::priority_queue<ContextPtr, std::vector<ContextPtr>, Less> storage_;
    std::mutex mtx_;
    size_t max_capacity_ = 4096;
};

bool SortModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "SortModule");
    sort_buffer_size_ = conf.sort().sort_buffer_size();
    delay_ms_ = conf.sort().delay_ms();
    lru_.reset(new LRU());
    LOG(INFO) << "Init " << this->name();
    return true;
}

void SortModule::start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) {
    auto consumer = [&]() {
        while (true) {
            auto ctx = lru_->pop();
            if (ctx == nullptr) {
                sleep(1);
                continue;
            }
            auto report_ms = get_ctx_timestamp(ctx);
            if (get_time_ms() < report_ms) {
                LOG(ERROR) << "now time is less than report time.[" << get_time_ms() << ", " << report_ms
                           << "]. Ignore message id: " << ctx->traceid();
                continue;
            }
            while (get_time_ms() - report_ms < delay_ms_) {
                LOG(INFO) << "Now time: " << get_time_ms() << ", report ms: " << report_ms
                          << ", deplay_ms: " << delay_ms_ << ", lru queue size: " << lru_->size()
                          << ", id: " << ctx->traceid();

                sleep(1);
            }
            out_que.put(ctx);
        }
    };
    std::thread con(consumer);
    con.detach();

    while (true) {
        auto ctx = in_que.get();
        while (false == lru_->put(ctx)) {
            LOG(WARNING) << "LRU queue is full.";
            sleep(1);
        }
        if (ctx->quit_signal()) {
            while (lru_->size()) {
                LOG(INFO) << "Waiting for LRU consumer. lru size: " << lru_->size();
                sleep(1);
                continue;
            }
            return;
        }
    }
}


RegisterClass(Module, SortModule);
} // namespace bigoai
