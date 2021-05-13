#include "dispatcher.h"
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <mutex>
#include <thread>
#include "submodule/submodule.h"
#include "utils/common_utils.h"

using namespace bigoai;

bvar::LatencyRecorder g_dispacher_latency("bigoai.dispatcher", "all"); // only use in grafana

RegisterClass(Module, DispatcherModule);

bool SubModuleObjectPool::add_instance(std::string submodule_name, const SubModuleConfig &config) {
    std::shared_ptr<SubModule> ptr(CreateObject(SubModule, submodule_name));
    if (nullptr == ptr) {
        LOG(FATAL) << "Not found SubModule: " << submodule_name;
        return false;
    }
    auto instance_name = config.instance_name();
    if (false == ptr->init(config)) {
        LOG(ERROR) << "Failed to init instance: " << instance_name;
        return false;
    }
    if (pool_.find(instance_name) != pool_.end()) {
        LOG(ERROR) << "Duplicated instance name: " << instance_name << ", submodule_name: " << submodule_name;
        return false;
    }
    pool_[instance_name] = ptr;
    VLOG(10) << "Instance name: " << instance_name;
    return true;
}

std::shared_ptr<SubModule> SubModuleObjectPool::get_instance(const std::string &instance_name) {
    if (pool_.find(instance_name) == pool_.end()) {
        return nullptr;
    }
    return pool_[instance_name];
}

bool DispatcherModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "DispatcherModule");

    if (!conf.has_dispatcher()) {
        return false;
    }

    batch_size_ = conf.dispatcher().batch_size();
    block_queue_timeout_ms_ = conf.dispatcher().block_queue_timeout_ms();
    work_thread_num_ = conf.work_thread_num();
    for (auto it : conf.dispatcher().submodule_conf()) {
        std::string submodule = it.name();
        auto config = it.conf();
        auto instance_name = config.instance_name();
        instance_names_.push_back(instance_name);
        instance_pools_.resize(work_thread_num_);
        for (auto i = 0; i < work_thread_num_; ++i) {
            if (false == instance_pools_[i].add_instance(submodule, config)) {
                LOG(ERROR) << "Failed to init submodel: " << submodule << ", instance name: " << instance_name;
                return false;
            }
        }
        metric_all_cnt_[instance_name] =
            std::shared_ptr<bvar::Adder<uint64_t>>(new bvar::Adder<uint64_t>("bigoai.model", instance_name + ".total"));
        metric_err_cnt_[instance_name] =
            std::shared_ptr<bvar::Adder<uint64_t>>(new bvar::Adder<uint64_t>("bigoai.model", instance_name + ".error"));
        metric_hit_cnt_[instance_name] =
            std::shared_ptr<bvar::Adder<uint64_t>>(new bvar::Adder<uint64_t>("bigoai.model", instance_name + ".hit"));
        metric_req_latency_[instance_name] = std::shared_ptr<bvar::LatencyRecorder>(
            new bvar::LatencyRecorder("bigoai.model", instance_name + ".latency"));
    }
    metric_batch_stat_.reset(new bvar::IntRecorder("bigoai.module", conf.instance_name() + ".batch.size"));


    LOG(INFO) << "Init " << name();
    return true;
}

bool DispatcherModule::do_context(std::vector<ContextPtr> &ctxs, SubModuleObjectPool &ins_pool) {
    VLOG(10) << "DoContext: " << name();

    std::mutex mutex;
    std::vector<std::thread> pool;
    int all_success = true;
    auto beg = get_time_ms();
    auto caller = [&](std::string instance_name) {
        *metric_all_cnt_[instance_name] << ctxs.size();
        std::shared_ptr<SubModule> p_ins = ins_pool.get_instance(instance_name);

        if (p_ins == nullptr) {
            LOG(FATAL) << "Not found instance object: " << p_ins;
            *metric_err_cnt_[instance_name] << 1;
            return;
        }

        if (batch_size_ == 1) {
            if (false == p_ins->call_service(ctxs[0])) {
                LOG(ERROR) << "Failed to call_service submodule: " << instance_name << "[ID:" << ctxs[0]->traceid()
                           << "]";
                all_success = false;
            }
            std::lock_guard<std::mutex> lock(mutex);
            if (false == p_ins->post_process(ctxs[0])) {
                LOG(ERROR) << "Failed to post_process submodule: " << instance_name << "[ID:" << ctxs[0]->traceid()
                           << "]";
                all_success = false;
            }
        } else {
            if (false == p_ins->call_service_batch(ctxs)) {
                LOG(ERROR) << "Failed to call_service_batch submodule: " << instance_name;
                all_success = false;
            }
            std::lock_guard<std::mutex> lock(mutex);
            if (false == p_ins->post_process_batch(ctxs)) {
                LOG(ERROR) << "Failed to post_process_batch submodule: " << instance_name;
                all_success = false;
            }
        }

        *metric_err_cnt_[instance_name] << p_ins->get_err_num();
        *metric_hit_cnt_[instance_name] << p_ins->get_hit_num();
        *metric_req_latency_[instance_name] << get_time_ms() - beg;
        p_ins->clear_metric();
    };

    for (size_t i = 0; i < instance_names_.size(); ++i) {
        pool.emplace_back(caller, instance_names_[i]);
    }

    for (auto &&th : pool) {
        th.join();
    }
    return all_success;
}

ContextPtr DispatcherModule::get_next_context(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que,
                                              int timeout_ms) {
    return in_que.get_timeout(timeout_ms);
}

void DispatcherModule::start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) {
    auto th = [&](SubModuleObjectPool ins_pool) {
        bool quit_signal = false;
        while (!quit_signal) {
            std::vector<ContextPtr> ctxs;
            for (int i = 0; !quit_signal && i < batch_size_; ++i) {
                auto ctx = get_next_context(in_que, out_que, block_queue_timeout_ms_);
                if (ctx == nullptr) {
                    LOG_IF(INFO, ctxs.size() != 0) << "Get empty context. batch size: " << ctxs.size();
                    break;
                }
                if (ctx->quit_signal() == true) {
                    quit_signal = true;
                    break;
                }
                if (ctx->traceid().size() == 0 || ctx->enable_modules().find(name()) == ctx->enable_modules().end()) {
                    VLOG(10) << "Skip module[" << name()
                             << "] for not found module name in need_module field. [ID:" << ctx->traceid() << "]";
                    out_que.put(ctx);
                    --i;
                    continue;
                }
                ctxs.emplace_back(ctx);
            }
            if (ctxs.size() == 0)
                continue;
            auto beg = get_time_ms();
            *metric_cnt_ << 1;
            *metric_que_size_ << out_que.size();
            *metric_batch_stat_ << ctxs.size();
            if (false == do_context(ctxs, ins_pool)) {
                *metric_err_ << ctxs.size();
            }
            *metric_latency_ << get_time_ms() - beg;
            for (size_t i = 0; i < ctxs.size(); ++i) {
                set_context_timestamp(ctxs[i], "after_inference");
            }
            for (auto ctx : ctxs) {
                out_que.put(ctx);
            }
        }
    };

    std::vector<std::thread> pool;
    for (int i = 0; i < work_thread_num_; ++i) {
        pool.emplace_back(th, instance_pools_[i]);
    }
    for (int i = 0; i < work_thread_num_; ++i) {
        pool[i].join();
    }
    LOG(INFO) << "Module[" << name() << "] quit.";
}
