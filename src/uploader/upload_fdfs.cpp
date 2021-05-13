#include "upload_fdfs.h"
#include <brpc/channel.h>
#include <brpc/server.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sys/types.h>
#include <mutex>
#include <string>
#include "utils/codec_util.h"
#include "util.pb.h"

#define AUTH_EXPIRE 240
#define _500M 524288000

namespace bigoai {


class TokenManager {
public:
    static TokenManager* get_instance() {
        static TokenManager token;
        return &token;
    }

    std::string get_fdfs_token(const std::string& policystr, const std::string& secret) {
        std::string policy_base64 = base64_encode((const unsigned char*)policystr.c_str(), policystr.size());

        unsigned char digest[1024] = {0};
        unsigned int digest_len = 0;

        if (HMAC(EVP_sha1(), secret.c_str(), secret.size(), (const unsigned char*)policy_base64.c_str(),
                 policy_base64.size(), digest, &digest_len) == NULL) {
            return "";
        }

        std::string signed_policy((const char*)digest, digest_len);

        std::string signed_policy_base64 =
            base64_encode((const unsigned char*)signed_policy.c_str(), signed_policy.size());

        return signed_policy_base64 + ":" + policy_base64;
    }

    std::string get_token(std::string bucket, uint32_t expire_interval, uint32_t file_expire_interval,
                          std::string secret) {
        // Fdfs protocal ref: http://wiki.bigo.sg:8090/pages/viewpage.action?pageId=22060387
        uint32_t now = time(NULL);
        std::lock_guard<std::mutex> lock(mutex_);
        if (token_expire_ < now + 60 || token_.size() == 0) {
            // Uploader Token
            FdfsToken pb;
            pb.set_bucket(bucket);
            token_expire_ = expire_interval + now;
            pb.set_expires(token_expire_);
            if (file_expire_interval != 0) {
                pb.set_fileexpire(file_expire_interval + now);
            }
            pb.add_fsizelimit(10);
            pb.add_fsizelimit(_500M);
            pb.add_mimelimit("audio/x-wav");

            std::string pb_json;
            json2pb::ProtoMessageToJson(pb, &pb_json);
            token_ = "BIGO " + get_fdfs_token(pb_json, secret);
            LOG(INFO) << "Policy: " << pb_json << ", Token: " << token_
                      << "; New token. Expire time: " << token_expire_;
        }
        return token_;
    }

private:
    TokenManager() {}
    std::string token_;
    uint64_t token_expire_ = 0;
    std::mutex mutex_;
};


bool FastDfsUploadModule::init(const ModuleConfig& conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());

    if (!conf.has_fast_dfs_upload()) {
        return false;
    }
    work_thread_num_ = conf.work_thread_num();
    channel_manager_.set_timeout_ms(conf.fast_dfs_upload().timeout_ms());
    channel_manager_.set_lb(conf.fast_dfs_upload().lb());
    bucket_ = conf.fast_dfs_upload().bucket();
    expire_interval_ = conf.fast_dfs_upload().expire_interval();
    file_expire_interval_ = conf.fast_dfs_upload().file_expire_interval();
    secret_ = conf.fast_dfs_upload().secret();
    url_ = conf.fast_dfs_upload().url() + "?bucket=" + bucket_;

    LOG(INFO) << "Init " << this->name();
    return true;
}

bool FastDfsUploadModule::do_context(ContextPtr& ctx) {
    CHECK_EQ(ctx->normalization_msg().data().voice_size(), 1);
    if (ctx->normalization_msg().appid() == "test") {
        return true;
    }
    bool need_upload = false;
    for (auto it : ctx->normalization_msg().data().voice(0).detailmlresult()) {
        if (it.second == "fail") {
            continue;
        } else if (it.second != "pass" && it.first != "voice_activity" && it.first != "lid_arabic") {
            need_upload = true;
            break;
        }
    }
    std::string audio_url;
    if (need_upload) {
        std::string wav_audio = get_wav_audio(ctx);
        if (wav_audio.size() == 0) {
            LOG(ERROR) << "wav audio is empty. ID:" << ctx->traceid();
            return true;
        }
        if (0 != upload2Fdfs(wav_audio, audio_url)) {
            LOG(FATAL) << "Failed to upload file [ID: " << ctx->traceid() << "]";
            return false;
        } else {
            (*ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(0)->mutable_subextradata())["wav_url"] =
                audio_url;
        }
    }
    return true;
}

int FastDfsUploadModule::upload2Fdfs(const std::string& imageData, std::string& imgUrl) {
    std::string token =
        TokenManager::get_instance()->get_token(bucket_, expire_interval_, file_expire_interval_, secret_);
    auto channel = channel_manager_.get_channel(url_);
    brpc::Controller cntl;
    cntl.http_request().set_content_type("audio/x-wav");
    cntl.http_request().SetHeader("Authorization", token);
    cntl.http_request().uri() = url_;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    cntl.request_attachment().append(imageData);
    FdfsResp resp;
    channel->CallMethod(NULL, &cntl, NULL, &resp, NULL);
    if (cntl.Failed()) {
        LOG(FATAL) << "Failed to upload image. Error text: " << cntl.ErrorText() << ", token: " << token
                   << ", file size: " << imageData.size();
        return -1;
    }
    imgUrl = resp.url();
    return 0;
}

RegisterClass(Module, FastDfsUploadModule);
} // namespace bigoai
