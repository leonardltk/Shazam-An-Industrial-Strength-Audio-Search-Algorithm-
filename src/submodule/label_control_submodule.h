#pragma once
#include "submodule/submodule.h"
#include <sstream>
#include <cstdlib>
#include <ctime>


namespace bigoai {

class LabelControlSubModule : public SubModule {
public:
    LabelControlSubModule();
    ~LabelControlSubModule() {}
    std::string name() const override { return "LabelControlSubModule"; }
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

    // use to control the output label
    std::vector<int> mapping_indices_list_;
    std::vector<std::string> aggragation_types_list_;
    std::vector<int> selected_indices_list_;
    int mapped_class_num_;
    std::string mapped_model_keys_;
    std::vector<std::string> mapped_model_keys_list_;
};

} // namespace bigoai
