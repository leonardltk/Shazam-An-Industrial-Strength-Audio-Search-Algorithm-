#include "multi_image.h"

using namespace bigoai;

RegisterClass(SubModule, MultiImageSubModule);

bool MultiImageSubModule::init(const SubModuleConfig& conf) {
    url_ = conf.rpc().url();
    auto lb = conf.rpc().lb();
    auto retry_time = conf.rpc().max_retry();
    auto timeout_ms = conf.rpc().timeout_ms();

    channel_manager_.set_timeout_ms(timeout_ms);
    channel_manager_.set_retry_time(retry_time);
    channel_manager_.set_lb(lb);
    for (auto threshold : conf.thresholds()) {
        threshold_manager_.add_rule(threshold);
    }
    models_.insert(models_.end(), conf.enable_model().begin(), conf.enable_model().end());

    CHECK_GE(models_.size(), 1u);
    return true;
}
