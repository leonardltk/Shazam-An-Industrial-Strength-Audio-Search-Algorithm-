#include <json2pb/pb_to_json.h>
#include <json2pb/json_to_pb.h>
#include "submodule/like_video/like_low_quality_ocr_based.h"

using namespace bigoai;

bool LikeLowQualityOcrBasedSubModule::init(const SubModuleConfig &conf) {
    set_name(conf.instance_name());

    if (conf.extra_data_size() != 1 || conf.extra_data(0).key() != "base_op") {
        return false;
    }
    base_op_ = conf.extra_data(0).val();


    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}

LikeOcrInfo get_ocr_text(ContextPtr &ctx) {
    LikeOcrInfo ret;
    if (ctx->normalization_msg().data().pic(0).modelinfo().find("cover_ocr") !=
        ctx->normalization_msg().data().pic(0).modelinfo().end()) {
        auto ocr_resp =
            (*ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(0)->mutable_modelinfo())["cover_ocr"];
        VLOG(10) << "OCR resp:" << ocr_resp;
        if (false == json2pb::JsonToProtoMessage(ocr_resp, &ret)) {
            LOG(ERROR) << "Failed to parse ocr response.[ID:" << ctx->traceid() << "]";
        }
    } else if (ctx->normalization_msg().data().video(0).modelinfo().find("cover_ocr") !=
               ctx->normalization_msg().data().video(0).modelinfo().end()) {
        auto ocr_resp =
            (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())["cover_ocr"];
        VLOG(10) << "OCR resp:" << ocr_resp;
        if (false == json2pb::JsonToProtoMessage(ocr_resp, &ret)) {
            LOG(ERROR) << "Failed to parse ocr response.[ID:" << ctx->traceid() << "]";
        }
    }
    return ret;
}

bool LikeLowQualityOcrBasedSubModule::call_service(ContextPtr &ctx) {
    resp_.Clear();
    // 1.0 pre filter
    if (ctx->video_src() != 0 && ctx->video_src() != 4) {
        VLOG(10) << "low_quality_orc_based src filter. src: " << ctx->video_src() << ". [ID:" << ctx->traceid() << "]";
        is_success_ = false;
        return true;
    }

    // 2.0 Set request.
    Request req;
    req.set_post_id(ctx->traceid());
    req.set_op(base_op_); //"copy_interanl","text_full","all"
    auto ocr_resp = get_ocr_text(ctx);
    if (ocr_resp.text().size() == 0) {
        VLOG(10) << "Skip " << name() << ", ocr text is empty.[ID:" << ctx->traceid() << "]";
        is_success_ = false;
        return true;
    }
    *req.mutable_ocrinfo() = ocr_resp;


    std::string ret_str;
    json2pb::ProtoMessageToJson(req, &ret_str);

    req.set_url(ctx->normalization_msg().data().video(0).content());
    req.add_images(ctx->raw_images(0));
    req.mutable_vinfo()->set_height(ctx->decode_info().height());
    req.mutable_vinfo()->set_width(ctx->decode_info().width());

    // 3.0 Call service
    is_success_ = call_http_service(ctx, channel_manager_, url_, req, resp_, max_retry_);
    if (is_success_ == false) {
        add_err_num();
    }
    return is_success_;
}


// enable rewrite.
bool LikeLowQualityOcrBasedSubModule::post_process(ContextPtr &ctx) {
    if (!is_success_) {
        return true;
    }
    const std::string model_name = "copy_internal";
    (*ctx->mutable_model_score())[model_name] = std::to_string(resp_.copy_statu());
    if (resp_.copy_statu()) {
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult())[model_name] =
            MODEL_RESULT_REVIEW;
        add_hit_num();
    } else {
        (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_detailmlresult())[model_name] =
            MODEL_RESULT_PASS;
    }
    std::string ret_str;
    json2pb::ProtoMessageToJson(resp_, &ret_str);
    (*ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0)->mutable_modelinfo())[model_name] = ret_str;
    return true;
}

RegisterClass(SubModule, LikeLowQualityOcrBasedSubModule);