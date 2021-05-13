#pragma once
#include "keywords_search_v2.pb.h"
#include "inner_keywords.pb.h"
#include "submodule/submodule.h"
#include <locale>
#include <codecvt>

namespace bigoai {
class GulfAsrSubModule : public SubModule {
public:
    GulfAsrSubModule();
    ~GulfAsrSubModule() {}
    std::string name() const override { return "GulfAsrSubModule"; }
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    bool call_keywords(ContextPtr ctx, std::string asr_text, keywords::Response &word_response);
    std::string convert_buckwalter_to_arabic(const std::string &buckwalter);

private:
    bool need_asr_;
    AsrResponse asr_resp_;
    keywords::Response word_resp_;
    ChannelManager channel_manager_;
    brpc::Channel word_channel_;
    std::string url_;
    std::string keyword_url_;
    int max_retry_;
    int keyword_max_retry_;
    std::string appid_;
    int businessid_;
    int text_type_;
    const std::map<char, wchar_t> buckwalter_to_arabic = {
        {'\'', L'\u0621'}, {'|', L'\u0622'}, {'>', L'\u0623'}, {'&', L'\u0624'}, {'<', L'\u0625'}, {'}', L'\u0626'},
        {'A', L'\u0627'},  {'b', L'\u0628'}, {'p', L'\u0629'}, {'t', L'\u062A'}, {'v', L'\u062B'}, {'j', L'\u062C'},
        {'H', L'\u062D'},  {'x', L'\u062E'}, {'d', L'\u062F'}, {'*', L'\u0630'}, {'r', L'\u0631'}, {'z', L'\u0632'},
        {'s', L'\u0633'},  {'$', L'\u0634'}, {'S', L'\u0635'}, {'D', L'\u0636'}, {'T', L'\u0637'}, {'Z', L'\u0638'},
        {'E', L'\u0639'},  {'g', L'\u063A'}, {'B', L'\u0640'}, {'f', L'\u0641'}, {'q', L'\u0642'}, {'k', L'\u0643'},
        {'l', L'\u0644'},  {'m', L'\u0645'}, {'n', L'\u0646'}, {'h', L'\u0647'}, {'w', L'\u0648'}, {'Y', L'\u0649'},
        {'y', L'\u064A'},  {'F', L'\u064B'}, {'N', L'\u064C'}, {'K', L'\u064D'}, {'a', L'\u064E'}, {'u', L'\u064F'},
        {'i', L'\u0650'},  {'~', L'\u0651'}, {'o', L'\u0652'}, {'`', L'\u0670'}, {'{', L'\u0671'}, {'P', L'\u067E'},
        {'J', L'\u0686'},  {'V', L'\u06A4'}, {'G', L'\u06AF'}};
};
}
