#pragma once
#include <map>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "utils/common_utils.h"

namespace bigoai {
inline std::string ignore_model_by_effect_impl(const std::map<std::string, std::vector<int>> &config,
                                        const rapidjson::Value &effect_data) {
    if (!effect_data.IsObject())
        return "";
    for (auto itr = config.begin(); itr != config.end(); ++itr) {
        std::string effect_name = itr->first;
        if (!effect_data.HasMember(effect_name.c_str()))
            continue;

        const char *effect_values = effect_data[effect_name.c_str()].GetString();
        if (effect_values[0] == '\0' || std::strcmp(effect_values, "-1") == 0)
            continue;

        std::vector<std::string> used_effect_values = split(effect_data[effect_name.c_str()].GetString(), '|');
        auto beg = itr->second.begin(), end = itr->second.end();
        for (std::string val : used_effect_values) {
            auto find_res = std::find(beg, end, std::stoi(val));
            if (find_res != end) {
                std::string ret = effect_name + "_" + val;
                return ret;
            }
        }
    }
    return "";
}

inline std::string ignore_model_by_effect(const std::map<std::string, std::vector<int>> &conf, const ContextPtr &ctx) {
    rapidjson::Document effect_data;
    effect_data.Parse(ctx->effect_data().c_str());

    std::string ignore_comb = ignore_model_by_effect_impl(conf, effect_data);
    if (ignore_comb != "")
        return ignore_comb;

    if (effect_data.HasMember("other_value")) {
        rapidjson::Document other_value;
        other_value.Parse(effect_data["other_value"].GetString());
        return ignore_model_by_effect_impl(conf, other_value);
    }

    if (effect_data.HasMember("otherMagic")) {
        rapidjson::Document other_magic;
        other_magic.Parse(effect_data["otherMagic"].GetString());
        return ignore_model_by_effect_impl(conf, other_magic);
    }

    return "";
}

}