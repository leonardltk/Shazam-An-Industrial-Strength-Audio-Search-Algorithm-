#pragma once

#include <brpc/channel.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>
#include <fstream>
#include "module/dispatcher.h"
#include "proxy.pb.h"
#include "util.pb.h"


namespace bigoai {

class VideoDispatcherModule : public DispatcherModule {
public:
    std::string name() const override { return "VideoDispatcherModule"; }
    ContextPtr get_next_context(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out, int timeout_ms) override;
};

} // namespace bigoai;
