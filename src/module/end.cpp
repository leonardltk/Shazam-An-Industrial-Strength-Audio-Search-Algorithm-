#include "module/end.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <atomic>

bvar::LatencyRecorder g_audit_latency("bigoai_audit_latency");

namespace bigoai {

bool EndModule::init(const ModuleConfig &conf) {
    LOG(INFO) << "Init EndModule.";
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    work_thread_num_ = 1;
    return true;
}

void EndModule::start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) {
    while (true) {
        auto ctx = in_que.get();
        if (ctx->quit_signal())
            break;
        if (ctx->traceid().size() != 0) {
            log_context_normalization_msg(ctx);
            g_audit_latency << (get_time_ms() - ctx->start_time());
        }
    }

    LOG(INFO) << "Module[" << name() << "] quit.";
}

// bool EndModule::do_context(ContextPtr &ctx, BlockingQueue<ContextPtr> &out_buf) {
//    g_audit_latency << (get_time_ms() - ctx->start_time());
//    return true;
// }

RegisterClass(Module, EndModule);
} // namespace bigoai
