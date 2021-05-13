#pragma once
#include "submodule/submodule.h"
#include <sstream>
#include <cstdlib>
#include <ctime>


namespace bigoai {

class BasicSubModule : public SubModule {
public:
    BasicSubModule();
    ~BasicSubModule() {}
    std::string name() const override { return "BasicSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx, const ImageResponse &resp);
    bool call_service_batch(std::vector<ContextPtr> &ctx) override;
    bool post_process_batch(std::vector<ContextPtr> &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    PictureResponse resp_;
    std::vector<int> fail_;
    
    ChannelManager channel_manager_;
    ThresholdManager threshold_manager_;
    std::string url_;
    std::string appid_;
    std::string model_types_;
    int class_num_;
    std::string model_keys_;
    std::vector<std::string> model_keys_list_;
    int normal_label_index_;
};

} // namespace bigoai
