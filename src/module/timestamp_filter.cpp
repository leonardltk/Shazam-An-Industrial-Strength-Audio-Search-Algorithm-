#include "module/timestamp_filter.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <thread>
#include "util.pb.h"
#include "utils/base64.h"
#include "utils/common_utils.h"


namespace bigoai {
bool TimestampFilterModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "TimestampFilterModule");
    if (!conf.has_timestamp_filter()) {
        return false;
    }

    work_thread_num_ = conf.work_thread_num();
    latest_timestamp_ms_ = conf.timestamp_filter().latest_timestamp_ms();
    earliest_timestamp_ms_ = conf.timestamp_filter().earliest_timestamp_ms();
    expire_ms_ = conf.timestamp_filter().expire_ms();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool TimestampFilterModule::do_context(ContextPtr &ctx) {
    uint64_t report_time = ctx->normalization_msg().reporttime();
    if (latest_timestamp_ms_ != 0) {
        if (report_time > latest_timestamp_ms_) {
            LOG(INFO) << "The message timestamp bigger than lastest_timestamp_ms.[" << report_time << ", "
                      << latest_timestamp_ms_ << "] " << ctx->traceid();
            discard_context(ctx);
            return true;
        }
    }
    if (earliest_timestamp_ms_ != 0) {
        if (report_time < earliest_timestamp_ms_) {
            LOG(INFO) << "The message timestamp less than lastest_timestamp_ms.[" << report_time << ", "
                      << earliest_timestamp_ms_ << "] " << ctx->traceid();
            discard_context(ctx);
            return true;
        }
    }
    if (expire_ms_ != 0) {
        auto tp = get_time_ms();
        if (tp <= report_time) {
            LOG(ERROR) << "host timestamp[" << tp << "] bigger than report time[" << report_time
                       << "], may be ntp service is down. Check host date config plz.";
            return true;
        }
        if (tp - report_time > expire_ms_) {
            LOG(ERROR) << "Message is expired. expire_ms: " << expire_ms_ << ", report_time: " << report_time
                       << ", now: " << tp << ", diff: " << (tp - report_time) / 1000 << "s.";
            discard_context(ctx);
            ctx->set_traceid(""); // also disable end module.
            return true;
        }
    }
    return true;
}

RegisterClass(Module, TimestampFilterModule);
} // namespace bigoai
