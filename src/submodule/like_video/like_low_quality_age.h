#pragma once

#include "submodule/submodule.h"
#include "util.pb.h"

namespace bigoai {
class LikeLowQualityAgeSubModule : public SubModule {
public:
    bool call_service(ContextPtr &ctx) override;
    bool post_process(ContextPtr &ctx) override;
    bool init(const SubModuleConfig &) override;

private:
    ChannelManager channel_manager_;
    bool res_ = false;
    std::string url_;
    LowQualityResult resp_;
    std::string enable_model_;
    int max_retry_;
    // 3至13岁：
    std::set<std::string> country_0313_ = {"ID", // 印尼：1
                                           "US", "CA", // 北美：2
                                           "AR", "BO", "CL", "CO", "CR", "CU", "DO", "EC", "SV", "GQ", "GT",
                                           "HN", "MX", "NI", "PA", "PY", "PE", "ES", "UY", "VE", "AD", // 西语：21
                                           "BR", "PT", //  葡语：2
                                           "GB", "NZ", "AU", "IE", "PR", "MT", // 欧英：6
                                           "DE", "AT", "LI", "BE", "CH", "IS", "NL", "CZ", "SK", "PL", "RO",
                                           "HR", "GR", "NO", "FI", "DK", "SE", "HU", "LU", "BA", "BG", "RS",
                                           "MK", "AL", "ME", "SI", "MD", // 中欧：27
                                           "FR", "MC", "HT", "IT", "SM", "VA"}; // 欧意法：6

    // 3至9岁：
    std::set<std::string> country_0309_ = {
        "RU", "UA", "BY", "KZ", "TJ", "KG", "UZ",  "TM", "AM", "GE", "AZ", "LV", "LT", "EE", // 俄语：14
        "AE", "BH", "KW", "OM", "QA", "SA", "DZ",  "EG", "IQ", "JO", "LB", "LY", "MA", "PS",
        "TN", "YE", "SO", "SD", "MR", "DJ", "COM", "IR", "TR", "SO", "IL", "CY"}; //中东：海湾6国+中东其他20;
    bool skip_;
};
}