#include <brpc/channel.h>
#include <brpc/server.h>
#include <gflags/gflags.h>
#include <csignal>
#include <memory>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "module/module.h"
#include "util.pb.h"
#include "config.pb.h"
#include "utils/block_queue.h"
#include "version.h"

using namespace bigoai;

DEFINE_int32(port, 8000, "");
DEFINE_string(config_file, "conf.json", "");

bool g_quit = false;

void signal_handler(int signal) {
    LOG(INFO) << "Recived quit signal. Ready for quit...";
    g_quit = true;
}

GlobalConfig global_config;

bool load_config(GlobalConfig &global_config) {
    LOG(FATAL) << __func__ << "() : FLAGS_config_file = " << FLAGS_config_file;
    auto config_fp = std::fopen(FLAGS_config_file.c_str(), "r");
    if (config_fp == nullptr) {
        LOG(FATAL) << "not found config file! " << FLAGS_config_file;
        return false;
    }

    int config_fd = fileno(std::fopen(FLAGS_config_file.c_str(), "r"));
    google::protobuf::io::FileInputStream config_file(config_fd);
    config_file.SetCloseOnDelete(true);
    if (!google::protobuf::TextFormat::Parse(&config_file, &global_config)) {
        LOG(FATAL) << "Parse config file failure! " << FLAGS_config_file;
        return false;
    }

    // auto fill instance_name
    for (int i = 0; i < global_config.module_conf_size(); ++i) {
        auto config = global_config.module_conf(i).conf();
        std::string module_name = global_config.module_conf(i).name();
        if (global_config.module_conf(i).conf().instance_name().size() == 0) {
            global_config.mutable_module_conf(i)->mutable_conf()->set_instance_name(module_name);
        }
        if (!config.has_dispatcher()) {
            continue;
        }
        for (auto j = 0; j < config.dispatcher().submodule_conf_size(); ++j) {
            std::string submodule = config.dispatcher().submodule_conf(j).name();
            auto conf = config.dispatcher().submodule_conf(j).conf();
            auto instance_name = conf.instance_name();
            if (instance_name.size() == 0) {
                LOG(WARNING) << "SubModule[" << submodule << "] use default instance name: " << submodule;
                global_config.mutable_module_conf(i)
                    ->mutable_conf()
                    ->mutable_dispatcher()
                    ->mutable_submodule_conf(j)
                    ->mutable_conf()
                    ->set_instance_name(submodule);
            }
        }
    }
    return true;
}


int main(int argc, char *argv[]) {
    LOG(INFO) << "GIT HASH: " << GIT_HASH;
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    brpc::StartDummyServerAt(FLAGS_port);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::map<int, BlockingQueue<ContextPtr>> buffer_queue;

    if (false == load_config(global_config)) {
        LOG(ERROR) << "Failed to load config.";
        return -1;
    }


    for (int i = 0; i < global_config.module_conf_size() + 1; ++i) {
        buffer_queue.emplace(std::make_pair(i, BlockingQueue<ContextPtr>{}));
    }

    std::vector<std::shared_ptr<Module>> mods;
    std::vector<std::thread> pool;

    std::vector<std::string> modules;
    for (int i = 0; i < global_config.module_conf_size(); ++i) {
        std::string module_name = global_config.module_conf(i).name();
        auto config = global_config.module_conf(i).conf();

        std::shared_ptr<Module> pModule(CreateObject(Module, module_name));

        if (nullptr == pModule) {
            LOG(FATAL) << "Not found modules: " << module_name;
            return 0;
        }

        mods.emplace_back(pModule);

        if (true != mods.back()->init(config)) {
            LOG(FATAL) << "Failed to init module : " << module_name;
            return 0;
        }
        modules.push_back(mods.back()->name());
    }

    for (int i = 0; i < global_config.module_conf_size(); ++i) {
        auto config = global_config.module_conf(i).conf();
        BlockingQueue<ContextPtr> &in_que = buffer_queue[i];
        BlockingQueue<ContextPtr> &out_que = buffer_queue[i + 1];
        in_que.set_max_size(config.queue_size());
        pool.emplace_back(std::thread([i, &in_que, &out_que, &mods]() { mods[i]->start(in_que, out_que); }));
    }

    // Waiting for quit.
    while (!g_quit) {
        ContextPtr ctx(new Context());
        for (size_t i = 0; i < modules.size(); ++i) {
            if (!global_config.module_conf(i).conf().enabled())
                continue;
            (*ctx->mutable_enable_modules())[modules[i]] = 1;
        }
        buffer_queue[0].put(ctx);
    }


    // clear queue
    while (buffer_queue[0].get_timeout(10) != nullptr);

    for (size_t i = 0; i < modules.size(); ++i) {
        LOG(INFO) << "Waiting for " << mods[i]->name() << " quit...";
        mods[i]->quit(buffer_queue[i]);
        pool[i].join();
    }
    LOG(INFO) << "Exit main function.";
}
