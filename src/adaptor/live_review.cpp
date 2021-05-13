#include "adaptor/live_review.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <gflags/gflags.h>
#include <openssl/md5.h>
#include <json2pb/pb_to_json.h>
#include "utils/common_utils.h"
#include "live_adaptor.pb.h"

using namespace bigoai;

DEFINE_int32(live_review_timeout_ms, 1500, "");
DEFINE_int32(live_review_work_thread, 128, "");
DEFINE_int32(live_review_retry_time, 1, "");
DEFINE_string(live_review_callback_url, "", "");
DEFINE_string(live_review_secret_key, "", "");

RegisterClass(Module, LiveReviewModule);

ChannelManager LiveReviewModule::channel_manager_;

enum LiveStatus { NORMAL = 3, DANGER = 4, FAILED = 5 };

std::string md5sum(const std::string &data, bool upper = false) {
    unsigned char md[16];
    MD5((const unsigned char *)data.c_str(), data.size(), md);
    std::stringstream md5ss;
    for (int i = 0; i < 16; ++i) {
        md5ss << std::setbase(16) << std::setw(2) << std::setfill('0') << int(md[i]);
    }
    std::string res = md5ss.str();
    if (upper) {
        transform(res.begin(), res.end(), res.begin(), ::toupper);
    }
    return res;
}

int get_message_status(AuditDataSubType data) {
    int status = LiveStatus::FAILED;
    for (auto it : data.detailmlresult()) {
        std::string model_name = it.first;
        std::string model_staus = it.second;
        if (it.second == "review" || it.second == "block") {
            status = LiveStatus::DANGER;
            break;
        } else if (it.second == "pass") {
            status = LiveStatus::NORMAL;
        }
    }
    return status;
}

bool LiveReviewModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    work_thread_num_ = FLAGS_live_review_work_thread;
    channel_manager_.set_timeout_ms(FLAGS_live_review_timeout_ms);
    channel_manager_.set_lb("rr");
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool LiveReviewModule::do_context(ContextPtr &ctx) {
    auto res = call_service(ctx);
    (*ctx->mutable_normalization_msg()->mutable_timestamp())["after_callback"] = get_time_ms();
    return res;
}


bool LiveReviewModule::call_service(ContextPtr &ctx) {
    CHECK_EQ(ctx->normalization_msg().data().live_size(), 0);

    auto norm_msg = ctx->normalization_msg();
    LiveReviewRequest live_msg;
    live_msg.set_machineid(norm_msg.appid());
    live_msg.set_reporttime(norm_msg.reporttime());
    live_msg.set_uid(norm_msg.uid());
    live_msg.set_country(norm_msg.country());
    live_msg.set_pictureurl(norm_msg.data().live(0).content());
    live_msg.set_reason(1); // 1:机审 2:用户举报
    live_msg.set_status(get_message_status(norm_msg.data().live(0)));

    live_msg.set_authsign(md5sum(live_msg.machineid() + live_msg.uid() + std::to_string(live_msg.reporttime()) +
                                 FLAGS_live_review_secret_key));
    live_msg.mutable_extpar()->set_sid(norm_msg.data().live(0).sid());

    auto attachinfo = live_msg.mutable_attachinfo();
    attachinfo->set_discardrate(0);
    for (auto it : norm_msg.data().live(0).modelinfo()) {
        std::string model_name = it.first;
        std::string model_res = it.second;
        (*attachinfo->mutable_modelresults())[model_name] = model_res;
        (*attachinfo->mutable_modelversions())[model_name] = "None";
    }

    std::string callback_url = FLAGS_live_review_callback_url;
    auto channel = channel_manager_.get_channel(callback_url);
    if (channel == nullptr) {
        LOG(ERROR) << "Faeild to init channel. url: " << callback_url << "[ID:" << ctx->traceid() << "]";
        return false;
    }

    bool flag = false;
    for (int i = 0; i < FLAGS_live_review_retry_time; ++i) {
        brpc::Controller cntl;
        cntl.http_request().SetHeader("traceid", ctx->traceid());
        cntl.http_request().uri() = callback_url;
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        LiveReviewResponse resp;
        channel->CallMethod(NULL, &cntl, &live_msg, &resp, NULL);
        if (cntl.Failed() || resp.code() != 0) {
            LOG(ERROR) << "Fail to call live review service, " << cntl.ErrorText().substr(0, 500) << "[" << name()
                       << "]"
                       << ", resposne code: " << resp.code() << ", code: " << cntl.ErrorCode()
                       << ", remote_side: " << cntl.remote_side() << ", retried time: " << i << "[" << ctx->traceid()
                       << "]";
            continue;
        }
        flag = true;
        break;
    }

    return flag;
}
