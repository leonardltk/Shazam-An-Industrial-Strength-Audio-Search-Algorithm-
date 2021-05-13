// #include "submodule/audio_core.h"
#include "audio_audit_shash.h"

using namespace bigoai;
brpc::Channel HelloShashSubModule::channel_;

RegisterClass(SubModule, HelloShashSubModule);

bool HelloShashSubModule::init(const SubModuleConfig &conf) {
    channel_manager_.set_timeout_ms(conf.rpc().timeout_ms());
    channel_manager_.set_lb(conf.rpc().lb());
    url_ = conf.rpc().url();
    max_retry_ = conf.rpc().max_retry();
    auto channel = channel_manager_.get_channel(url_);
    if (channel == nullptr) {
        LOG(ERROR) << __func__ << " Failed init channel! " << name() << " " << url_;
        return false;
    }

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool HelloShashSubModule::call_service(ContextPtr &ctx) {

    /* Push Ratio Plan */
        float r, push_ratio;
        // push_ratio = 0.95;
        push_ratio = 0.00;
        r = ((double)std::rand() / (RAND_MAX));
        LOG(ERROR) << "------ " << __func__ << " ------";
        LOG(ERROR) << __func__ << "() : the random numer is " << r;
        if ( r < push_ratio ) {
            LOG(ERROR) << __func__ << "() : not push since probability, skip, the push rate is " << push_ratio;
            return true;
        }
        LOG(ERROR) << __func__ << "() : Pushed " << push_ratio;

    /* Check available pcm */
        if (ctx->pcm_audio().size() == 0) {
            LOG(ERROR) << __func__ << "() : Miss pcm data. [TraceID:" << ctx->traceid() << "]";
            // return false;
        }

    /* Init Params */
    shash_audit_pb_.Clear();
    ShashRequest req;

    /* Get details */
        /* Refer to 江唯杰 John's ./src/submodule/gulf_asr.cpp */
        try {
            LOG(INFO) << __func__ << "() : -------- Request req  ---------------";
            LOG(INFO) << __func__ << "() : -------- Setting in these ---------------";
            LOG(INFO) << __func__ << "() : set_op                       : query";
            LOG(INFO) << __func__ << "() : ctx->raw_audio() has size    :" << ctx->raw_audio().size() << " (Not used)";
            LOG(INFO) << __func__ << "() : ctx->pcm_audio() has size    : " << ctx->pcm_audio().size();
            LOG(INFO) << __func__ << "() : ctx->traceid()               : " << ctx->traceid();
            LOG(INFO) << __func__ << "() : ctx->url_audio()             : " << ctx->url_audio();
            LOG(INFO) << __func__ << "() : get_context_url(ctx)         : " << get_context_url(ctx);
            LOG(INFO) << __func__ << "() : ctx->normalization_msg().data().voice(0).content()   : " << ctx->normalization_msg().data().voice(0).content();
            LOG(INFO) << __func__ << "() : ctx->normalization_msg().data().voice(0).id()        : " << ctx->normalization_msg().data().voice(0).id();
            req.set_op( "query" );
            req.set_data( ctx->pcm_audio() );
            req.set_data_type( ShashRequest::PCM_S16_SR16000 );
            req.set_uid( ctx->normalization_msg().uid() );
            req.set_post_id( ctx->normalization_msg().data().voice(0).id() );
            /* ------------- */
            /* for now use uid, instead of the real id */
            req.set_id( ctx->normalization_msg().uid() );
            // req.set_id( ctx->normalization_msg().data().voice(0).id() );
            /* ------------- */
            req.set_url( ctx->url_audio() );
            req.set_label( ctx->traceid() );
            LOG(INFO) << __func__ << "() : ------- Double checking req params ---------------";
            LOG(INFO) << __FUNCTION__ << ": req->op()       : " << "query";
            LOG(INFO) << __FUNCTION__ << ": req->data()     : ctx->pcm_audio()";
            LOG(INFO) << __FUNCTION__ << ": req->data_type(): ShashRequest::PCM_S16_SR16000";
            LOG(INFO) << __FUNCTION__ << ": req->uid()      : " << ctx->normalization_msg().uid();
            LOG(INFO) << __FUNCTION__ << ": req->id()       : " << ctx->normalization_msg().data().voice(0).id();
            LOG(INFO) << __FUNCTION__ << ": req->post_id()  : " << ctx->normalization_msg().data().voice(0).id();
            LOG(INFO) << __FUNCTION__ << ": req->url()      : " << ctx->url_audio();
            LOG(INFO) << __FUNCTION__ << ": req->label()    : " << ctx->traceid();
            // LOG(INFO) << __func__ << "() : req.op()     : " << req.op() ;
            LOG(INFO) << __func__ << "() : ------- Request req (Done) ---------------";
        } catch (std::exception& e) {
            LOG(INFO) << __func__ << "() : e.what() = "<< e.what();
            return false;
        }

    /* 3.0 Call service */
        LOG(INFO) << __func__ << " -------- 3.0 Call service ---------------";
        brpc::Controller cntl;
        auto channel = channel_manager_.get_channel(url_);
        LOG(INFO) << __func__ << "() : channel : " << channel;
        success_ = false;
        std::string result_string;
        for (int retry = 0; !success_ && retry <= max_retry_; ++retry) {
            // 3.0 Call service
            LOG(INFO) << __func__ << "() : 3.0 Call service | retry = " << retry;
            cntl.http_request().uri() = url_;
            cntl.http_request().set_method(brpc::HTTP_METHOD_POST);

            channel->CallMethod(NULL, &cntl, &req, &shash_audit_pb_, NULL);

            /* Convert the result to string */
            LOG(INFO) << __func__ << "() : Convert the result to string";
            std::string shash_audit_pb_str;
            json2pb::ProtoMessageToJson(shash_audit_pb_, &shash_audit_pb_str);
            LOG(INFO) << __func__ << "() : check cntl.Failed() = " << cntl.Failed();
            if (cntl.Failed()) {
                log_request_failed(cntl, ctx->traceid(), retry);
                LOG(INFO) << "() : Calling shash audit service failed. [uid:" << req.uid() << "], [url:" << req.url()
                           << "], [status_code:" << cntl.http_response().status_code()
                           << "], [resp: " << cntl.response_attachment().to_string() << "], [result: " << shash_audit_pb_str
                           << "]";
                continue;
            }

            LOG(INFO) << __func__ << "() : cntl.http_response().status_code() = " << cntl.http_response().status_code();
            if (cntl.http_response().status_code() == 200 || cntl.http_response().status_code() == 203) {
                VLOG(10) << "() : Calling shash audit service succeed. [uid:" << req.uid() << "], [url:" << req.url()
                         << "], [status_code:" << cntl.http_response().status_code()
                         << "], [resp: " << cntl.response_attachment().to_string() << "], [result: " << shash_audit_pb_str
                         << "]";
                success_ = true;
                return success_;
            }
        }
        LOG(INFO) << __func__ << "\n-------- 3.0 Call service (Done) ---------------";

    if (!success_) {
        add_err_num(1);
    }
    return success_;
}

bool HelloShashSubModule::call_service_batch(std::vector<ContextPtr> &ctx) {
    for (auto &c : ctx) {
        call_service(c);
    }
    return true;
}

bool HelloShashSubModule::post_process_batch(std::vector<ContextPtr> &ctxs) {
    for (size_t i = 0; i < ctxs.size(); ++i) {
        post_process(ctxs[i]);
    }
    return true;
}

bool HelloShashSubModule::post_process(ContextPtr &ctx) {

    if (success_ == false || context_is_download_failed(ctx)) {
        set_context_modelinfo(ctx, "audio_audit_shash", "fail but pass.");
    } else {
        set_context_modelinfo(ctx, "audio_audit_shash", "succeed.");
    }
    return true;
}
