#include "submodule/like_video/like_watermark.h"

using namespace bigoai;

bool LikeWatermarkSubModule::init(const SubModuleConfig &conf) {
    // 1.0 Init channel

    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    return true;
}

bool LikeWatermarkSubModule::call_service(ContextPtr &ctx) {
    skip_ = true;
    if (ctx->video_src() == 1 || ctx->video_src() == 2 || ctx->video_src() == 3) {
        VLOG(10) << "Watermark filter. src: " << ctx->video_src() << ". [ID:" << ctx->traceid() << "]";
        return true;
    }
    watermark_info_.Clear();

    // 2.0 Set request.
    Request req;
    CHECK_EQ(ctx->normalization_msg().data().video_size(), 1);
    req.set_url(ctx->normalization_msg().data().video(0).content());
    req.set_post_id(ctx->traceid());
    req.set_op("query");

    // 3.0 Call service without retry.
    WatermarkWrap resp;
    skip_ = false;
    call_http_service(ctx, channel_manager_, url_, req, resp, max_retry_);
    auto it = resp.results().find("like-watermark-audit");
    if (it == resp.results().end()) {
        LOG(ERROR) << "Not found like-watermark-audit field. [ID:" << ctx->traceid() << "]";
        add_err_num();
        return false;
    }
    watermark_info_ = it->second;
    // 4.0 Parse result.
    return true;
}

// enable rewrite.
bool LikeWatermarkSubModule::post_process(ContextPtr &ctx) {
    if (skip_)
        return true;

    const std::string model_name = "copy";
    auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(0);
    (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_FAIL;

    std::string ret_str;
    json2pb::ProtoMessageToJson(watermark_info_, &ret_str);
    (*video->mutable_modelinfo())[model_name] = ret_str;

    std::ostringstream os;
    os << "{\"comp_basic\":";  // 0:无违规水印，1 低准确率水印，2 普通水印，3 竞品水印

    if (watermark_info_.xgb_score() > 1) {
        (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
        add_hit_num();
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        os << std::to_string(watermark_info_.xgb_score()) << ",";   //xgb_score: 0 无水印；1 低准确率水印(包含竞品和波普)；2 普通水印；3 竞品水印
        for (auto name : watermark_info_.cls_names()) {
            os << "\"" << name << "\":\"1\",";
        }
        os.seekp(-1, std::ios_base::end); // Delete last ","
        os << "}";
        (*ctx->mutable_model_score())[model_name] = os.str();
        VLOG(10) << "Watermark:" << os.str();
        return true;
    }

    //retrieval_scores: 0 竞品旧；1 竞品新；2 普通旧；3 普通新；4 likee
    if (watermark_info_.max_retrieval_scores().size() >= 5 &&
        (watermark_info_.max_retrieval_scores(0) > thd1_ || watermark_info_.max_retrieval_scores(2) > thd1_ ||
         watermark_info_.max_retrieval_scores(1) > thd2_ || watermark_info_.max_retrieval_scores(3) > thd2_)) {
        (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_REVIEW;
        add_hit_num();
        (*ctx->mutable_normalization_msg()->mutable_extradata())["resultCode"] = "4";
        //comp_basic socore set
        if (watermark_info_.max_retrieval_scores(0) > thd1_  || watermark_info_.max_retrieval_scores(1) > thd2_ )
            os << "\"3\",";
        else if (watermark_info_.max_retrieval_scores(2) > thd1_  || watermark_info_.max_retrieval_scores(3) > thd2_ )
            os << "\"2\",";
        else
            os << "\"0\",";
    } else {
        (*video->mutable_detailmlresult())[model_name] = MODEL_RESULT_PASS;
        os << "\"0\",";
    }

    for (auto name : watermark_info_.retrieval_result()) {
        os  << "\""<< name.clsname() << "\":\"" << name.score() << "\",";
    }
    os.seekp(-1, std::ios_base::end); // Delete last ","
    os << "}";
    (*ctx->mutable_model_score())[model_name] = os.str();
    VLOG(10) << "Watermark:" << os.str();
    return true;
}

RegisterClass(SubModule, LikeWatermarkSubModule);