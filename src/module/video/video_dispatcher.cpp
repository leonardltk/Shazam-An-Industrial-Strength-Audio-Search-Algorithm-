#include "module/video/video_dispatcher.h"
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <mutex>
#include <thread>
#include "submodule/submodule.h"
#include "utils/common_utils.h"

using namespace bigoai;
DEFINE_string(video_dispatcher_enable_submodule, "", "");
DEFINE_int32(video_dispatcher_thread_num, 32, "");
DEFINE_int32(video_dispatcher_batch_size, 1, "");

RegisterClass(Module, VideoDispatcherModule);


ContextPtr VideoDispatcherModule::get_next_context(BlockingQueue<ContextPtr> &in_que,
                                                   BlockingQueue<ContextPtr> &out_que, int timeout_ms) {
    while (true) {
        auto ctx = in_que.get_timeout(100);
        if (ctx == nullptr || ctx->quit_signal())
            return ctx;
        if (ctx->normalization_msg().data().video_size() == 0) {
            if (ctx->traceid().size()) {
                LOG(INFO) << name() << " ignore message. The number of video is 0. [ID:" << ctx->traceid() << "]";
                out_que.put(ctx);
            }
        } else {
            return ctx;
        }
    }
}
