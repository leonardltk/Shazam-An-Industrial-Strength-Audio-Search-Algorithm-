#include "module/image/image_dispatcher.h"
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <mutex>
#include <thread>
#include "submodule/submodule.h"
#include "utils/common_utils.h"

using namespace bigoai;

RegisterClass(Module, ImageDispatcherModule);

ContextPtr ImageDispatcherModule::get_next_context(BlockingQueue<ContextPtr> &in_que,
                                                   BlockingQueue<ContextPtr> &out_que, int timeout_ms) {
    while (true) {
        auto ctx = in_que.get_timeout(timeout_ms);
        if (ctx == nullptr || ctx->quit_signal())
            return ctx;
        if (ctx->normalization_msg().data().pic_size() == 0) {
            if (ctx->traceid().size()) {
                LOG(INFO) << "Ignore message. The number of pic is 0. [ID:" << ctx->traceid() << "]";
                out_que.put(ctx);
            }
        } else {
            return ctx;
        }
    }
}
