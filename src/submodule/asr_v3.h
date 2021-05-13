#pragma once
#include "keywords_search_v2.pb.h"
#include "inner_keywords.pb.h"
#include "submodule/submodule.h"


namespace bigoai {
class AsrV3SubModule : public SubModule {
public:
    AsrV3SubModule();
    ~AsrV3SubModule() {}
    std::string name() const override { return "AsrV3SubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    bool call_keywords(ContextPtr ctx, std::string ocr_text, keywords::Response &nlp_response);

private:
    bool need_asr_;
    AsrResponse asr_resp_;
    keywords::Response word_resp_;

    ChannelManager channel_manager_;
    brpc::Channel nlp_channel_;
    std::string url_;
    std::string nlp_url_;
    int max_retry_;
    int nlp_max_retry_;
    std::string appid_;
    int businessid_;
    int text_type_;
};

}
