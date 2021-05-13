#include "multi_image_dispatcher.h"
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <mutex>
#include <thread>
#include "butil/logging.h"
#include "module/content.h"
#include "normalization_message.pb.h"
#include "submodule/submodule.h"
#include "util.pb.h"
#include "utils/common_utils.h"

using namespace bigoai;

RegisterClass(Module, MultiImageDispatcherModule);


bool MultiImageDispatcherModule::do_context(std::vector<ContextPtr> &ctxs, SubModuleObjectPool &ins_pool) {
    VLOG(10) << "DoContext: " << name();

    std::mutex mutex;
    std::vector<std::thread> pool;
    int all_success = true;
    std::vector<ContextPtr> pCtxs; // Use this to split a ctx into multiple.
    int nPic = ctxs[0]->normalization_msg().data().pic_size();
    auto beg = get_time_ms();

    // Split context.
    for (int i = 0; i < nPic; ++i) {
        ContextPtr pCtx(new Context());
        pCtx->CopyFrom(*ctxs[0]);

        pCtx->mutable_normalization_msg()->mutable_data()->clear_pic();
        AuditDataSubType *pic = pCtx->mutable_normalization_msg()->mutable_data()->add_pic();
        pic->CopyFrom(ctxs[0]->normalization_msg().data().pic(i));

        pCtx->clear_raw_images();
        pCtx->add_raw_images(ctxs[0]->raw_images(i));
        pCtxs.emplace_back(pCtx);
    }

    auto caller = [&](std::string instance_name) {
        *metric_all_cnt_[instance_name] << ctxs.size();
        std::shared_ptr<SubModule> p_ins = ins_pool.get_instance(instance_name);

        if (p_ins == nullptr) {
            LOG(FATAL) << "Not found instance object: " << p_ins;
            *metric_err_cnt_[instance_name] << 1;
            return;
        }

        for (int pic_i = 0; pic_i < nPic; ++pic_i) {
            if (false == p_ins->call_service(pCtxs[pic_i])) {
                LOG(ERROR) << "Failed to call_service submodule: " << instance_name;
                all_success = false;
            }
            std::lock_guard<std::mutex> lock(mutex);
            if (false == p_ins->post_process(pCtxs[pic_i])) {
                LOG(ERROR) << "Failed to post_process submodule: " << instance_name;
                all_success = false;
            }
        }


        *metric_err_cnt_[instance_name] << p_ins->get_err_num();
        *metric_hit_cnt_[instance_name] << p_ins->get_hit_num();
        *metric_req_latency_[instance_name] << get_time_ms() - beg;
        p_ins->clear_metric();
    };

    for (size_t i = 0; i < instance_names_.size(); ++i) {
        pool.emplace_back(caller, instance_names_[i]);
    }

    for (auto &&th : pool) {
        th.join();
    }

    // Merge contexts.
    ctxs[0]->mutable_normalization_msg()->mutable_data()->clear_pic();
    for (auto pCtx : pCtxs) {
        AuditDataSubType *pic = ctxs[0]->mutable_normalization_msg()->mutable_data()->add_pic();
        pic->CopyFrom(pCtx->normalization_msg().data().pic(0));
    }

    return all_success;
}
