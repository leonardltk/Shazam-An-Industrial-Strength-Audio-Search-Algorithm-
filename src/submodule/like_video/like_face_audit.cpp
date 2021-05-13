#include "submodule/like_video/like_face_audit.h"
namespace bigoai {

RegisterClass(SubModule, LikeFaceAuditImageModule);

bool LikeFaceAuditImageModule::init(const SubModuleConfig &conf) {
    set_name(conf.instance_name());

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}

bool LikeFaceAuditImageModule::call_service(ContextPtr &ctx) {
    face_audit_pb_.Clear();

    Request req;
    req.set_op("query_write");
    req.set_image(ctx->raw_images(0));
    req.set_post_id(ctx->traceid());
    req.set_uid(ctx->normalization_msg().uid());
    req.set_url(ctx->normalization_msg().data().pic(0).content());
    req.set_country(ctx->normalization_msg().country());
    req.set_appid("4802"); // likee video cover

    res_ = call_http_service(ctx, channel_manager_, url_, req, face_audit_pb_, max_retry_);
    if (res_ == false) {
        add_err_num();
    }
    return res_;
}

bool LikeFaceAuditImageModule::post_process(ContextPtr &ctx) {
    if (!res_)
        return false;
    std::string ret_str;
    json2pb::ProtoMessageToJson(face_audit_pb_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())["LikeFaceAudit"] =
        ret_str;
    return true;
}

}