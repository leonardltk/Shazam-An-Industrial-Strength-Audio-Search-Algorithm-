#pragma once

#include "submodule/submodule.h"
#include "util.pb.h"

namespace bigoai {
class LikeLowQualityGeneralSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;
};
}