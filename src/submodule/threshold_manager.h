#pragma once
#include <map>
#include "utils/common_utils.h"
#include "config.pb.h"
namespace bigoai {

class ThresholdManager {
public:
    ThresholdManager() {
        VLOG(10) << "Init threadholdManager.";
        thresholds_["default"]["default"] = 0;
    }

    bool add_rule(std::string thresholds_rule) {
        thresholds_["default"]["default"] = 0;
        for (auto rule : split(thresholds_rule, ',')) {
            auto pair = split(rule, ':');
            if (pair.size() == 2) {
                auto country = pair[0];
                auto threshold = std::stof(pair[1]);
                thresholds_["default"][country] = threshold;
            } else if (pair.size() == 3) {
                auto model = pair[0];
                auto country = pair[1];
                auto threshold = std::stof(pair[2]);
                LOG(INFO) << "Add threshold. model: " << model << ", country: " << country
                          << ", threshold: " << threshold;
                thresholds_[model][country] = threshold;
            } else {
                LOG(INFO) << "Failed to parse threshold_rule: " << rule;
                return false;
            }
        }
        for (auto it : thresholds_) {
            std::string model = it.first;
            auto m = it.second;
            if (m.count("default") == 0) {
                LOG(ERROR) << "model: " << model << " not set default threshold.";
                return false;
            }
        }
        return true;
    }

    bool add_rule(config::ThresholdConfig rule) {
        std::string model = rule.model();
        for (auto country : rule.country()) {
            thresholds_[model][country] = rule.threshold();
            VLOG(10) << "Add threshold. model: " << model << ", country: " << country
                     << ", threshold: " << rule.threshold();
        }
        return true;
    }

    float get_threshold(std::string country = "default", std::string model = "default") {
        if (thresholds_.find(model) != thresholds_.end()) {
            if (thresholds_[model].find(country) != thresholds_[model].end()) {
                return thresholds_[model][country];
            } else {
                return thresholds_[model]["default"];
            }
        }
        return thresholds_["default"]["default"];
    }

    std::vector<std::string> get_model_names() {
        std::vector<std::string> ret;
        for (auto it : thresholds_) {
            if (it.first == "default")
                continue;
            ret.push_back(it.first);
        }
        return ret;
    }

private:
    std::map<std::string, std::map<std::string, float>> thresholds_; // model, country, thresholds;
};
} // namespace bigoai
