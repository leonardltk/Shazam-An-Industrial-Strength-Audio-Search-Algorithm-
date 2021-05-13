#pragma once

#include "image_dispatcher.h"


namespace bigoai {

class MultiImageDispatcherModule : public ImageDispatcherModule {
public:
    std::string name() const override { return "MultiImageDispatcherModule"; }
    bool do_context(std::vector<ContextPtr> &ctxs, SubModuleObjectPool &ins_pool) override;
};
}
