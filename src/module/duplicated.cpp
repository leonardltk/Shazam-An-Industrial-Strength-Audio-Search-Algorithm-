#include "module/duplicated.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <thread>
#include "util.pb.h"


namespace bigoai {
bool DuplicatedModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "DuplicatedModule");

    if (!conf.has_duplicated()) {
        return false;
    }

    pika_.reset(new PikaClient(conf.duplicated().timeout_ms(), conf.duplicated().password_index(),
                               conf.duplicated().expire_second(), conf.duplicated().hosts(),
                               conf.duplicated().prefix()));
    op_ = conf.duplicated().op();

    work_thread_num_ = conf.work_thread_num();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool DuplicatedModule::do_context(ContextPtr &ctx) {
    if (ctx->traceid().size() == 0) {
        return true;
    }

    if (op_ == "query") {
        if (pika_->is_exist(ctx->traceid())) {
            LOG(WARNING) << "Ignore duplicated message. ID:" << ctx->traceid();
            discard_context(ctx);
            ctx->set_traceid(""); // Disable end module.
        }
    } else if (op_ == "write") {
        if ((*ctx->mutable_normalization_msg()->mutable_extradata())["ml_review_callback"] != "1") {
            LOG(INFO) << "Duplicated ignore id: " << ctx->traceid();
            return true;
        }
        if (false == pika_->write(ctx->traceid())) {
            LOG(ERROR) << "Failed to write redis. ID:" << ctx->traceid();
        }
    } else {
        LOG(ERROR) << "Unknown op: " << op_;
    }
    return true;
}

RegisterClass(Module, DuplicatedModule);
} // namespace bigoai
