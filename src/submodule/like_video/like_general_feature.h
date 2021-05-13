#include "submodule/submodule.h"

namespace bigoai {
class LikeGeneralFeatureModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    std::string url_;
    int max_retry_;

    std::string ann_url_;
    std::string kakashi_url_;
    std::string lowq_url_;
    std::string ann_blacklist_url_;

    GeneralFeatureResponse resp_;
    GeneralFeatureResponse kakashi_resp_;
    GeneralFeatureWrap ann_resp_;
    GeneralFeatureWrap ann_blacklist_resp_;
    LowQualityResult lowq_resp_;
    bool skip_;
};
}
