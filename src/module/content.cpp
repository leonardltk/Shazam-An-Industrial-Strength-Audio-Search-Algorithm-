#include "module/content.h"
#include "utils/common_utils.h"
#include "json2pb/pb_to_json.h"

DEFINE_string(audit_type, "pic", "pic/text/video/voice");

namespace bigoai {

void log_context_normalization_msg(ContextPtr &ctx) {
    std::string ctx_str;
    json2pb::ProtoMessageToJson(ctx->normalization_msg(), &ctx_str);

    LOG(INFO) << ctx_str;
}

bool context_data_check(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().pic_size()) {
        return ctx->normalization_msg().data().pic_size() > 0 && ctx->raw_images_size() > 0;
    } else if (ctx->normalization_msg().data().live_size()) {
        return ctx->normalization_msg().data().live_size() > 0 && ctx->raw_images_size() > 0;
    } else if (ctx->normalization_msg().data().video_size()) {
        return ctx->normalization_msg().data().video_size() > 0 && !ctx->raw_video().empty();
    } else if (ctx->normalization_msg().data().text_size()) {
        return ctx->normalization_msg().data().text_size() > 0;
    } else if (ctx->normalization_msg().data().voice_size()) {
        return ctx->normalization_msg().data().voice_size() > 0 && !ctx->raw_audio().empty();
    }
    return true;
}

size_t get_context_data_size(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().pic_size()) {
        return ctx->normalization_msg().data().pic_size();
    } else if (ctx->normalization_msg().data().live_size()) {
        return ctx->normalization_msg().data().live_size();
    } else if (ctx->normalization_msg().data().video_size()) {
        return ctx->normalization_msg().data().video_size();
    } else if (ctx->normalization_msg().data().text_size()) {
        return ctx->normalization_msg().data().text_size();
    } else if (ctx->normalization_msg().data().voice_size()) {
        return ctx->normalization_msg().data().voice_size();
    }
    return 0;
}

const std::string get_context_url(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().pic_size()) {
        return ctx->normalization_msg().data().pic_size() > 0 ? ctx->normalization_msg().data().pic(0).content() : "";
    } else if (ctx->normalization_msg().data().live_size()) {
        return ctx->normalization_msg().data().live_size() > 0 ? ctx->normalization_msg().data().live(0).content() : "";
    } else if (ctx->normalization_msg().data().video_size()) {
        return ctx->normalization_msg().data().video_size() > 0 ? ctx->normalization_msg().data().video(0).content()
                                                                : "";
    } else if (ctx->normalization_msg().data().text_size()) {
        return ctx->normalization_msg().data().text_size() > 0 ? ctx->normalization_msg().data().text(0).content() : "";
    } else if (ctx->normalization_msg().data().voice_size()) {
        return ctx->normalization_msg().data().voice_size() > 0 ? ctx->normalization_msg().data().voice(0).content()
                                                                : "";
    }
    return "";
}

AuditDataSubType *get_context_data_object(ContextPtr &ctx) {
    if (ctx->normalization_msg().data().pic_size()) {
        return ctx->normalization_msg().data().pic_size() > 0
                   ? ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0)
                   : nullptr;
    } else if (ctx->normalization_msg().data().live_size()) {
        return ctx->normalization_msg().data().live_size() > 0
                   ? ctx->mutable_normalization_msg()->mutable_data()->mutable_live(0)
                   : nullptr;
    } else if (ctx->normalization_msg().data().video_size()) {
        return ctx->normalization_msg().data().video_size() > 0
                   ? ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)
                   : nullptr;
    } else if (ctx->normalization_msg().data().text_size()) {
        return ctx->normalization_msg().data().text_size() > 0
                   ? ctx->mutable_normalization_msg()->mutable_data()->mutable_text(0)
                   : nullptr;
    } else if (ctx->normalization_msg().data().voice_size()) {
        return ctx->normalization_msg().data().voice_size() > 0
                   ? ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)
                   : nullptr;
    }
    return nullptr;
}

AuditDataSubType *get_context_data_object_by_type(ContextPtr &ctx, const std::string type) {
    auto ptr = ctx->mutable_normalization_msg()->mutable_data();
    const google::protobuf::Descriptor *desc = ptr->GetDescriptor();
    const google::protobuf::FieldDescriptor *fdesc = desc->FindFieldByName(type);
    if (fdesc == nullptr) {
        return nullptr;
    }
    const google::protobuf::Reflection *ref = ptr->GetReflection();
    if (ref->FieldSize(ctx->normalization_msg().data(), fdesc) > 0) {
        return dynamic_cast<AuditDataSubType *>(ref->MutableRepeatedMessage(ptr, fdesc, 0));
    }
    return nullptr;
}

void set_context_detailmlresult_modelinfo(ContextPtr &ctx, const std::string &key, const std::string result,
                                          const std::string info) {
    auto obj = get_context_data_object(ctx);
    if (obj != nullptr) {
        (*obj->mutable_detailmlresult())[key] = result;
        (*obj->mutable_modelinfo())[key] = info;
    }
}

void set_context_detailmlresult_modelinfo(ContextPtr &ctx, const std::string &key, const std::string result,
                                          const std::string info, const std::string &type) {
    if (type == "")
        return set_context_detailmlresult_modelinfo(ctx, key, result, info);
    auto obj = get_context_data_object_by_type(ctx, type);
    if (obj != nullptr) {
        (*obj->mutable_detailmlresult())[key] = result;
        (*obj->mutable_modelinfo())[key] = info;
    }
}

void set_context_modelinfo(ContextPtr &ctx, const std::string &key, const std::string info) {
    auto obj = get_context_data_object(ctx);
    if (obj != nullptr) {
        (*obj->mutable_modelinfo())[key] = info;
    }
}

void set_context_detailmlresult_modelinfo_with_type(ContextPtr &ctx, const std::string &key, const std::string result,
                                                    const std::string info, const std::string type) {
    auto obj = get_context_data_object_by_type(ctx, type);
    if (obj != nullptr) {
        (*obj->mutable_detailmlresult())[key] = result;
        (*obj->mutable_modelinfo())[key] = info;
    }
}

void set_context_timestamp(ContextPtr &ctx, const std::string &key) {
    if (ctx != nullptr) {
        (*ctx->mutable_normalization_msg()->mutable_timestamp())[key] = get_time_ms();
    }
}

bool context_is_download_failed(ContextPtr &ctx) {
    auto obj = get_context_data_object(ctx);
    return (obj == nullptr || obj->subextradata().find("download") != obj->subextradata().end());
}

void discard_context(ContextPtr &ctx) { ctx->mutable_enable_modules()->clear(); }

void discard_context_without_log(ContextPtr &ctx) {
    ctx->set_traceid("");
    ctx->mutable_enable_modules()->clear();
}


}
