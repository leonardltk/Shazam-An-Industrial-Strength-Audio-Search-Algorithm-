#include "module/modify_fail_res.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <thread>
#include "util.pb.h"
#include "utils/base64.h"
#include "utils/common_utils.h"

namespace bigoai {

bool ModifyFailResModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "ModifyFailResModule");
    work_thread_num_ = conf.work_thread_num();
    if (!conf.has_modify_fail_res()) {
        return false;
    }
    model_names_.insert(model_names_.end(), conf.modify_fail_res().ignore_models().begin(),
                        conf.modify_fail_res().ignore_models().end());
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool ModifyFailResModule::do_context(ContextPtr &ctx) {
    bool flag = false;

    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); i++) {
        auto image = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
        for (auto model : model_names_) {
            if (image->detailmlresult().find(model) == image->detailmlresult().end())
                continue;
            if (image->detailmlresult().find(model)->second != "fail")
                continue;
            (*image->mutable_detailmlresult())[model] = "pass";
            flag = true;
            LOG(INFO) << "Ignore failed model[" << model << "][ID:" << ctx->traceid() << "]";
        }
    }

    for (int i = 0; i < ctx->normalization_msg().data().video_size(); i++) {
        auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(i);
        for (auto model : model_names_) {
            if (video->detailmlresult().find(model) == video->detailmlresult().end())
                continue;
            if (video->detailmlresult().find(model)->second != "fail")
                continue;
            (*video->mutable_detailmlresult())[model] = "pass";
            flag = true;
            LOG(INFO) << "Ignore failed model[" << model << "][ID:" << ctx->traceid() << "]";
        }
    }

    if (flag == true) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["ModifyFailResModule"] = "1";
    }
    return true;
}

RegisterClass(Module, ModifyFailResModule);
} // namespace bigoai
