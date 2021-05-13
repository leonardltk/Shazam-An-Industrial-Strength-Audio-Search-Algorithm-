#pragma once

#include <brpc/channel.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>
#include <fstream>
#include "module/module.h"
#include "proxy.pb.h"
#include "util.pb.h"


namespace bigoai {

class SubModuleObjectPool;
class DispatcherModule : public Module {
public:
    bool init(const ModuleConfig &conf) override;
    void start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) override;
    bool do_context(ContextPtr &ctx) override { throw "NotImplemented"; }

    virtual bool do_context(std::vector<ContextPtr> &ctx, SubModuleObjectPool &);
    virtual ContextPtr get_next_context(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out,
                                        int timeout_ms);


protected:
    std::map<std::string, std::shared_ptr<bvar::Adder<uint64_t>>> metric_all_cnt_;
    std::map<std::string, std::shared_ptr<bvar::Adder<uint64_t>>> metric_err_cnt_;
    std::map<std::string, std::shared_ptr<bvar::Adder<uint64_t>>> metric_hit_cnt_;
    std::map<std::string, std::shared_ptr<bvar::LatencyRecorder>> metric_req_latency_;
    std::shared_ptr<bvar::IntRecorder> metric_batch_stat_;

    int batch_size_;
    int block_queue_timeout_ms_;
    std::vector<std::string> instance_names_;
    std::vector<SubModuleObjectPool> instance_pools_;
};

class SubModule;
class SubModuleObjectPool {
public:
    SubModuleObjectPool() {}
    bool add_instance(std::string submodule_name, const SubModuleConfig &conf);
    std::shared_ptr<SubModule> get_instance(const std::string &instance_name);

private:
    std::map<std::string, std::shared_ptr<SubModule>> pool_;
};
} // namespace bigoai;
