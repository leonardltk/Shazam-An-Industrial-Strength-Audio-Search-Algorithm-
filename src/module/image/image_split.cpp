#include "module/image/image_split.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <atomic>
#include <thread>
#include "proxy.pb.h"
#include "utils/common_utils.h"


namespace bigoai {


bool ImageSplitModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    work_thread_num_ = conf.work_thread_num();
    LOG(INFO) << "Init " << name();
    return true;
}

void ImageSplitModule::start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) {
    auto th = [&]() {
        while (true) {
            auto ctx = in_que.get();
            if (ctx->quit_signal())
                return;
            if (ctx->enable_modules().find(name()) == ctx->enable_modules().end()) {
                LOG(INFO) << "Skip module[" << name()
                          << "] for not found module name in enable_modules field. [ID:" << ctx->traceid() << "]";
                out_que.put(ctx);
                continue;
            }
            auto beg = get_time_ms();
            for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
                auto pic = ctx->normalization_msg().data().pic(i);
                ContextPtr split_ctx(new Context);
                *split_ctx = *ctx;
                split_ctx->mutable_normalization_msg()->mutable_data()->mutable_pic()->Clear();
                *(split_ctx->mutable_normalization_msg()->mutable_data()->add_pic()) = pic;
                out_que.put(split_ctx);
            }
            auto cost = get_time_ms() - beg;
            update_metrics(true, out_que.size(), cost);
        }
    };
    std::vector<std::thread> pool;
    for (int i = 0; i < work_thread_num_; ++i) {
        pool.emplace_back(th);
    }
    for (int i = 0; i < work_thread_num_; ++i) {
        pool[i].join();
    }
    LOG(INFO) << "Module[" << name() << "] quit.";
}

RegisterClass(Module, ImageSplitModule);
} // namespace bigoai
