#include "module/downloader.h"
#include <brpc/callback.h>
#include <brpc/channel.h>
#include <json2pb/json_to_pb.h>
#include <json2pb/pb_to_json.h>
#include <thread>
#include "util.pb.h"
#include "utils/base64.h"
#include "utils/common_utils.h"


namespace bigoai {
bool DownloaderModule::init(const ModuleConfig &conf) {
    init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : "DownloaderModule");
    if (!conf.has_downloader()) {
        return false;
    }
    work_thread_num_ = conf.work_thread_num();
    channel_manager_.set_timeout_ms(conf.downloader().timeout_ms());
    channel_manager_.set_lb(conf.downloader().lb());
    channel_manager_.set_retry_time(conf.downloader().max_retry());
    enable_image_ = conf.downloader().enable_image();
    enable_voice_ = conf.downloader().enable_voice();
    enable_video_ = conf.downloader().enable_video();
    LOG(INFO) << "Init " << this->name();
    return true;
}

bool DownloaderModule::redirect_retry(std::shared_ptr<brpc::Channel> channel, const std::string &url,
                                      int redirect_count, std::string &content) {
    LOG(INFO) << "Redirect URL: " << url << ", redirect count: " << redirect_count;
    if (redirect_count > 3) {
        LOG(ERROR) << "The number of redirection more than 3!";
        return false;
    }
    if (url.substr(0, 4) == "http") {
        channel = channel_manager_.get_channel(url);
    }

    brpc::Controller cntl;
    cntl.http_request().uri() = url;
    channel->CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
    if (cntl.Failed()) {
        if (cntl.http_response().status_code() == 302) {
            return redirect_retry(channel, *cntl.http_response().GetHeader("Location"), redirect_count + 1, content);
        }
        LOG(ERROR) << "Failed to access url after 302. url: " << url;
        return false;
    }
    content = cntl.response_attachment().to_string();
    return true;
}


bool DownloaderModule::download_video(ContextPtr &ctx) {
    auto all_success = true;
    for (int i = 0; i < ctx->normalization_msg().data().video_size(); ++i) {
        auto video = ctx->mutable_normalization_msg()->mutable_data()->mutable_video(i);
        brpc::Controller cntl;
        cntl.http_request().uri() = video->content();
        std::shared_ptr<brpc::Channel> channel = channel_manager_.get_channel(video->content());

        auto success = false;
        std::string content;
        if (channel.get() == nullptr) {
            LOG(ERROR) << "Failed to init channel. url: " << video->content() << " [ID:" << ctx->traceid() << "]";
            success = false;
        } else {
            channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
            if (cntl.Failed()) {
                success = false;
                if (cntl.http_response().status_code() == 302) {
                    success = redirect_retry(channel, *cntl.http_response().GetHeader("Location"), 1, content);
                }
            } else {
                success = true;
                content = cntl.response_attachment().to_string();
            }
            if (false == success) {
                LOG(ERROR) << "Failed to download video. message will be discard. URL: " << video->content()
                           << " [ID:" << ctx->traceid() << "]";
            }
        }

        if (success) {
            ctx->set_raw_video(content);
        } else {
            all_success = false;
            ctx->set_raw_video("");
            video->set_failed_module(name());
        }
        (*video->mutable_subextradata())["download_status_code"] = std::to_string(cntl.http_response().status_code());
        (*video->mutable_subextradata())["download_size"] = std::to_string(ctx->raw_video().size());
    }
    return all_success;
}

bool DownloaderModule::download_image(ContextPtr &ctx) {
    auto all_success = true;
    for (int i = 0; i < ctx->normalization_msg().data().pic_size(); ++i) {
        auto pic = ctx->mutable_normalization_msg()->mutable_data()->mutable_pic(i);
        brpc::Controller cntl;
        cntl.http_request().uri() = pic->content();
        std::shared_ptr<brpc::Channel> channel = channel_manager_.get_channel(pic->content());

        std::string content;
        auto success = false;
        if (channel.get() == nullptr) {
            LOG(ERROR) << "Failed to init channel. url: " << pic->content() << " [ID:" << ctx->traceid() << "]";
        } else {
            channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
            if (cntl.Failed()) {
                if (cntl.http_response().status_code() == 302) {
                    success = redirect_retry(channel, *cntl.http_response().GetHeader("Location"), 1, content);
                }
            } else {
                success = true;
                content = cntl.response_attachment().to_string();
            }
            if (cntl.http_response().status_code() == 204) {
                LOG(ERROR) << "Empty image. Server return 204[No content].";
                success = false;
            }
        }

        if (false == success) {
            all_success = false;
            pic->set_failed_module(name());
            ctx->mutable_enable_modules()->clear();
            (*ctx->mutable_enable_modules())["UploadHiveModule"] = 1;
            (*ctx->mutable_enable_modules())["ReviewModule"] = 1;
            LOG(ERROR) << "Failed to download image. message will be discard. URL: " << pic->content()
                       << " [ID:" << ctx->traceid() << "]";
        }
        (*pic->mutable_subextradata())["download_status_code"] = std::to_string(cntl.http_response().status_code());
        ctx->add_raw_images(content);
    }
    return all_success;
}

bool DownloaderModule::download_voice(ContextPtr &ctx) {
    auto all_success = true;

    for (int i = 0; i < ctx->normalization_msg().data().voice_size(); ++i) {
        auto voice = ctx->mutable_normalization_msg()->mutable_data()->mutable_voice(i);
        ctx->set_traceid(ctx->traceid() + "_" + voice->id());
        ctx->set_url_audio(voice->content());

        brpc::Controller cntl;
        cntl.http_request().uri() = voice->content();
        std::shared_ptr<brpc::Channel> channel = channel_manager_.get_channel(voice->content());

        auto success = true;
        if (channel.get() == nullptr) {
            LOG(ERROR) << "Failed to init channel. url: " << voice->content() << " [ID:" << ctx->traceid() << "]";
            success = false;
        } else {
            channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
            if (cntl.Failed()) {
                LOG(ERROR) << "Failed to download voice. message will be discard. URL: " << voice->content()
                           << ", IP: " << cntl.remote_side() << " [ID:" << ctx->traceid() << "]";
                success = false;
            }
        }

        if (success) {
            ctx->set_raw_audio(cntl.response_attachment().to_string());
            std::ostringstream os;
            os << cntl.remote_side();
            (*voice->mutable_subextradata())["download_ip"] = os.str();
        } else {
            all_success = false;
            ctx->set_raw_audio("");
            voice->set_failed_module(name());
            (*voice->mutable_subextradata())["download_msg"] = cntl.ErrorText();
        }
        (*voice->mutable_subextradata())["download_status_code"] = std::to_string(cntl.http_response().status_code());
    }
    return all_success;
}

bool DownloaderModule::do_context(ContextPtr &ctx) {
    auto all_success = true;

    // 1.0 Download image
    if (enable_image_ && false == download_image(ctx)) {
        all_success = false;
    }

    // 2.0 Download audio
    if (enable_voice_ && false == download_voice(ctx)) {
        all_success = false;
    }

    // 3.0 Download video
    if (enable_video_ && false == download_video(ctx)) {
        all_success = false;
    }

    set_context_timestamp(ctx, "after_download");

    return all_success;
}

RegisterClass(Module, DownloaderModule);

} // namespace bigoai
