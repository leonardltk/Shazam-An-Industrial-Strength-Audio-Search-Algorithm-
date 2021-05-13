#include "module/isp_filter.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <thread>
#include "util.pb.h"
#include "utils/base64.h"
#include "utils/common_utils.h"

DEFINE_int32(isp_filter_pass_number, -1, "");
DEFINE_int32(isp_filter_thread_number, 1, "");
DEFINE_bool(isp_filter_sp_logic, false, "");

namespace bigoai {
bool IspFilterModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());

    if (!conf.has_isp_filter()) {
        return false;
    }

    work_thread_num_ = conf.work_thread_num();
    pass_isp_ = std::to_string(conf.isp_filter().pass_number());
    sp_logic_ = conf.isp_filter().sp_logic();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool IspFilterModule::do_context(ContextPtr &ctx) {
    if (ctx->normalization_msg().extradata().find("isp") == ctx->normalization_msg().extradata().end()) {
        LOG(INFO) << "Filter message. Not found isp field. [ID:" << ctx->traceid() << "]";
        discard_context(ctx);
        return true;
    }
    if (sp_logic_) {
        std::string isp = ctx->normalization_msg().extradata().at("isp");
        if (isp != "1" && isp != "2" && isp != "32") {
            return true;
        } else {
            discard_context(ctx);
        }
    } else if (pass_isp_ == ctx->normalization_msg().extradata().at("isp")) {
        return true;
    } else {
        discard_context(ctx);
        LOG(INFO) << "Filter message. isp: " << ctx->normalization_msg().extradata().at("isp")
                  << ", not equal to: " << pass_isp_ << "[ID:" << ctx->traceid() << "]";
    }
    return true;
}

RegisterClass(Module, IspFilterModule);
} // namespace bigoai
