#include "upload_hive.h"
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>
#include <map>
#include "utils/common_utils.h"

using namespace bigoai;

namespace {
template<typename T1, typename T2>
std::string to_json(const google::protobuf::Map<T1, T2> &in) {
    std::stringstream ss;
    ss << "{";
    for (auto it : in) {
        ss << "\"" << it.first << "\":\"" << it.second << "\",";
    }
    std::string ret = ss.str();
    if (ret.size() > 1) {
        ret[ret.size() - 1] = '}';
    } else {
        ret += "}";
    }
    return ret;
}

void add_uint64(bigoai::HiveElemUInt64 *elem, uint64_t value, int seq) {
    elem->set_value(value);
    elem->set_type(4);
    elem->set_seq(seq);
}

void add_string(bigoai::HiveElemString *elem, const std::string &value, int seq) {
    elem->set_value(value);
    elem->set_type(5);
    elem->set_seq(seq);
}

void add_mapstr(bigoai::HiveElemMapStr *elem, const std::map<std::string, std::string> &values, int seq) {
    int cnt = 1;
    for (auto it : values) {
        HiveElemValue pair;
        pair.set_value(it.second);
        pair.set_seq(cnt++);
        (*elem->mutable_value())[it.first] = pair;
    }
    elem->set_type(7);
    elem->set_seq(seq);
}
} // namespace

RegisterClass(Module, UploadHiveModule);

ChannelManager UploadHiveModule::channel_manager_;
extern GlobalConfig global_config;

std::string get_kafka_topic(const ModuleConfig &conf) {
    if (conf.upload_hive().kafka_topic().size() == 0) {
        for (int i = 0; i < global_config.module_conf_size(); ++i) {
            if (!global_config.module_conf(i).conf().has_kafka_consumer())
                continue;
            return global_config.module_conf(i).conf().kafka_consumer().topic(0);
        }
    }
    return conf.upload_hive().kafka_topic();
}

std::string get_kafka_groupid(const ModuleConfig &conf) {
    if (conf.upload_hive().kafka_groupid().size() == 0) {
        LOG(INFO) << "GLobal config module size: " << global_config.module_conf_size();
        for (int i = 0; i < global_config.module_conf_size(); ++i) {
            if (!global_config.module_conf(i).conf().has_kafka_consumer())
                continue;
            return global_config.module_conf(i).conf().kafka_consumer().groupid(0);
        }
    }
    return conf.upload_hive().kafka_groupid();
}

std::string get_service_name(const ModuleConfig &conf) {
    if (conf.upload_hive().service_name().size())
        return conf.upload_hive().service_name();
    std::string hostname = butil::my_hostname();
    auto field = split(hostname, '-');
    std::string k8s_deployname;
    for (int i = 0; i < (int)field.size() - 2; ++i) {
        k8s_deployname += field[i] + "-";
    }
    if (k8s_deployname.size() > 0) {
        k8s_deployname = k8s_deployname.substr(0, k8s_deployname.size() - 1);
    } else {
        k8s_deployname = hostname;
    }
    return k8s_deployname;
}

std::string get_module_list(const ModuleConfig &conf) {
    std::ostringstream os;
    char last_char = 0;
    std::string enable_modules;

    if (conf.upload_hive().enable_module().size()) {
        enable_modules = conf.upload_hive().enable_module();
    } else {
        for (int i = 0; i < global_config.module_conf_size(); ++i) {
            enable_modules += global_config.module_conf(i).conf().instance_name() + ",";
        }
        if (enable_modules.size() > 0) {
            enable_modules = enable_modules.substr(0, enable_modules.size() - 1);
        }
    }

    for (size_t i = 0; i < enable_modules.size(); ++i) {
        if (isupper(enable_modules[i])) {
            if (last_char != ',' && last_char != 0)
                os << "_";
            os << char(std::tolower(enable_modules[i]));
        } else {
            os << enable_modules[i];
        }
        last_char = enable_modules[i];
    }
    return os.str();
}

std::string get_submodule_list(const ModuleConfig &conf) {
    std::ostringstream os;
    char last_char = 0;
    std::string enable_submodule;
    if (conf.upload_hive().enable_submodule().size()) {
        enable_submodule = conf.upload_hive().enable_submodule();
    } else {
        for (int i = 0; i < global_config.module_conf_size(); ++i) {
            if (!global_config.module_conf(i).conf().has_dispatcher()) {
                continue;
            }
            auto dispatcher = global_config.module_conf(i).conf().dispatcher();
            for (int j = 0; j < dispatcher.submodule_conf_size(); ++j) {
                auto submodule_conf = dispatcher.submodule_conf(j).conf();
                enable_submodule += submodule_conf.instance_name() + ",";
            }
        }
        if (enable_submodule.size() > 0) {
            enable_submodule = enable_submodule.substr(0, enable_submodule.size() - 1);
        }
    }

    for (size_t i = 0; i < enable_submodule.size(); ++i) {
        if (isupper(enable_submodule[i])) {
            if (last_char != ',' && last_char != 0)
                os << "_";
            os << char(std::tolower(enable_submodule[i]));
        } else {
            os << enable_submodule[i];
        }
        last_char = enable_submodule[i];
    }
    return os.str();
}

bool UploadHiveModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
    if (!conf.has_upload_hive()) {
        return false;
    }
    work_thread_num_ = conf.work_thread_num();
    url_ = conf.upload_hive().url();

    channel_manager_.set_timeout_ms(conf.upload_hive().timeout_ms());
    channel_manager_.set_lb(conf.upload_hive().lb());
    channel_manager_.set_retry_time(conf.upload_hive().max_retry());

    auto channel = channel_manager_.get_channel(conf.upload_hive().url());
    if (channel == nullptr) {
        LOG(INFO) << "Failed init channel! " << name() << " " << conf.upload_hive().url();
        return false;
    }
    rewrite_business_type_ = conf.upload_hive().rewrite_business_type();

    // fill in extra info field
    extra_info_["ml_service"] = get_service_name(conf);
    extra_info_["ml_topic"] = get_kafka_topic(conf);
    extra_info_["ml_groupid"] = get_kafka_groupid(conf);
    extra_info_["ml_enable_modules"] = get_module_list(conf);
    extra_info_["ml_enable_submodule"] = get_submodule_list(conf);
    for (auto it = extra_info_.begin(); it != extra_info_.end(); ++it) {
        LOG(INFO) << "Upload hive extra info, " << it->first << ": " << it->second;
    }
    auto th = [&]() {
        while (1) {
            extra_info_flag_ = true;
            sleep(3600); // 1h
        }
    };
    std::thread daemon_thread(th);
    daemon_thread.detach();
    LOG(INFO) << "Init " << this->name();
    return true;
}

void UploadHiveModule::normalization_msg_to_hive_message(const NormaliztionAuditMessage &pb, StdAuditResultHive &ret) {
    add_string(ret.mutable_orderid(), pb.orderid(), 1);
    add_string(ret.mutable_uid(), pb.uid(), 2);
    add_string(ret.mutable_appid(), pb.appid(), 3);
    add_string(ret.mutable_businesstype(), pb.businesstype(), 4);
    add_string(ret.mutable_country(), pb.country(), 5);
    add_uint64(ret.mutable_reporttime(), pb.reporttime(), 6);
    add_string(ret.mutable_sourcetype(), pb.sourcetype(), 7);

    std::map<std::string, std::string> buf_map;
    for (auto it : pb.extradata()) {
        std::string key = it.first;
        buf_map[key] = it.second;
    }
    add_mapstr(ret.mutable_extradata(), buf_map, 8);

    std::string buf;
    json2pb::ProtoMessageToJson(pb.data(), &buf);
    add_string(ret.mutable_data(), buf, 9);

    add_string(ret.mutable_timestamp(), to_json(pb.timestamp()), 10);

    add_string(ret.mutable_callbackurl(), pb.callbackurl(), 11);
    const std::string extendinfo = std::string(GIT_HASH) + "[" + std::string(__DATE__) + "]";
    add_string(ret.mutable_extendinfo(), extendinfo, 12);

    buf_map.clear();

    auto ml_res_pb = pb.detailmlresult();
    if (pb.data().video_size()) {
        ml_res_pb = pb.data().video(0).detailmlresult();
    } else if (pb.data().voice_size()) {
        ml_res_pb = pb.data().voice(0).detailmlresult();
    } else if (pb.data().pic_size()) {
        ml_res_pb = pb.data().pic(0).detailmlresult();
    } else if (pb.data().live_size()) {
        ml_res_pb = pb.data().live(0).detailmlresult();
    } else {
        LOG(ERROR) << "Empty data. [ID:" << pb.orderid();
    }
    std::string final_status = "pass";
    for (auto it : ml_res_pb) {
        std::string key = it.first;
        std::string value = it.second;
        buf_map[key] = value;
        if (value == "review" || value == "block") {
            final_status = "review";
        } else if (final_status != "review" && value == "fail") {
            final_status = "fail";
        }
    }
    buf_map["status"] = final_status;
    add_mapstr(ret.mutable_detailmlresult(), buf_map, 13);
}

bool UploadHiveModule::do_context(ContextPtr &ctx) {
    if (extra_info_flag_) {
        extra_info_flag_ = false;
        LOG(INFO) << "Add extra info. [ID:" << ctx->traceid() << "]";
        for (auto it : extra_info_) {
            (*ctx->mutable_normalization_msg()->mutable_extradata())[it.first] = it.second;
        }
    }
    auto res = call_service(ctx);
    (*ctx->mutable_normalization_msg()->mutable_timestamp())["after_upload_hive"] = get_time_ms();
    return res;
}

bool UploadHiveModule::call_service(ContextPtr &ctx) {
    std::string url = url_;
    auto channel = channel_manager_.get_channel(url);
    if (channel == nullptr) {
        LOG(ERROR) << "Faeild to init channel. url: " << url << "[ID:" << ctx->traceid() << "]";
        return false;
    }
    if (rewrite_business_type_.size()) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["raw_buesiness_type"] =
            ctx->normalization_msg().businesstype();
        ctx->mutable_normalization_msg()->set_businesstype(rewrite_business_type_);
    }

    brpc::Controller cntl;
    cntl.http_request().uri() = url;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    cntl.set_always_print_primitive_fields(true);

    StdAuditResultHive hive_msg;
    normalization_msg_to_hive_message(ctx->normalization_msg(), hive_msg);

    std::string hive_str;
    json2pb::Pb2JsonOptions options;
    options.always_print_primitive_fields = true;
    json2pb::ProtoMessageToJson(hive_msg, &hive_str, options);

    channel->CallMethod(NULL, &cntl, &hive_msg, NULL, NULL);
    if (cntl.Failed()) {
        LOG(ERROR) << "Fail to call upload hive service, " << cntl.ErrorText().substr(0, 500) << "[" << name() << "]"
                   << ", code: " << cntl.ErrorCode() << "[" << ctx->traceid() << "]";
        LOG(ERROR) << "Baina: " << hive_str;
        return false;
    }
    VLOG(10) << name() << " id: " << ctx->normalization_msg().orderid()
             << " resp: " << cntl.response_attachment().to_string() << ", code: " << cntl.http_response().status_code();
    return true;
}
