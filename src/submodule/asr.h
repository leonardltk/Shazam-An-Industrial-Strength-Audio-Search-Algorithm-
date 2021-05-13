#pragma once
#include "keywords_search_v2.pb.h"
#include "inner_keywords.pb.h"
#include "submodule/submodule.h"


namespace bigoai {
class AsrSubModule : public SubModule {
public:
    AsrSubModule();
    ~AsrSubModule() {}
    std::string name() const override { return "AsrSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    bool call_keywords(ContextPtr ctx, std::string ocr_text, keywords::HitResultPb &response,
                       keywords::Response &nlp_response, keywords::PunctResponse &punct_response,
                       AsrResponse &asr_response);
    bool call_punct(ContextPtr ctx, std::string ocr_text, keywords::PunctResponse &punct_response,
                    std::string &punct_keywords_text, std::string &punct_nlp_text);

private:
    bool need_asr_;
    AsrResponse asr_resp_;
    keywords::Response nlp_resp_;
    keywords::HitResultPb keywords_resp_;
    keywords::PunctResponse punct_resp_;

    ChannelManager channel_manager_;
    brpc::Channel nlp_channel_;
    brpc::Channel keywords_channel_;
    brpc::Channel punct_channel_;
    std::string url_;
    std::string nlp_url_;
    std::string punct_url_;
    int max_retry_;
    int nlp_max_retry_;
    int punct_max_retry_;
    bool enable_punct_;

    int srcid_;
    int businessid_;
    std::string appid_;
    int text_type_;
};
}
