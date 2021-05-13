#ifndef PROXY_DOWNLOADER_H
#define PROXY_DOWNLOADER_H

#include <cstdint>
#include <string>
#include <vector>
#include "module/module.h"
#include "util.pb.h"
#include "utils/block_queue.h"

namespace bigoai {

class EndModule : public Module {
public:
    bool init(const ModuleConfig &) override;
    void start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) override;
    std::string name() const { return "EndModule"; }
};

}


#endif // PROXY_DOWNLOADER_H
