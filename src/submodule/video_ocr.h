#pragma once
#include "submodule/submodule.h"

namespace bigoai {

class VideoOcrSubModule : public SubModule {
public:
    VideoOcrSubModule();
    ~VideoOcrSubModule() {}
    std::string name() const override { return "VideoOcrSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

    bool text_audit(ContextPtr &ctx, const std::string &text, ImageOcrResult &ocr_result);

private:
    ChannelManager channel_manager_;
    ChannelManager channel_manager_text_;
    std::string appid_;
    int businessid_;
    int text_type_;
    int max_retry_;
    int text_max_retry_;
    std::string url_;
    std::string text_url_;
    std::string result_key_;
    bool check_level_;

    bool success_;
    std::vector<ImageOcrResult> ocr_results_;
};

} // namespace bigoai
