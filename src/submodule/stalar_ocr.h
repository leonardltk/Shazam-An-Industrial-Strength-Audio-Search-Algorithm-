#pragma once
#include "inner_keywords.pb.h"
#include "submodule/submodule.h"


namespace bigoai {
class StalarOcrSubModule : public SubModule {
public:
    StalarOcrSubModule();
    ~StalarOcrSubModule() {}
    std::string name() const override { return "StalarOcrSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    bool parse_ocr(const NormalizationOCRResponse &ocr_resp, const ContextPtr &ctx, keywords::Response &keyword_resp);
    bool call_keyword(const std::string &text, const std::string &uid, const std::string &country,
                      const std::string &traceid, keywords::Response &resp);

private:
    std::vector<NormalizationOCRResponse> ocr_resps_;
    std::vector<keywords::Response> keyword_resps_;

    ChannelManager channel_manager_;
    ChannelManager channel_manager_text_;
    std::string url_;
    std::string text_url_;
};

}
