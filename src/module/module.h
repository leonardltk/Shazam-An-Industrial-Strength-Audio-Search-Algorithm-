#pragma once
#include <bvar/bvar.h>
#include <iostream>
#include <string>

#include "module/content.h"
#include "utils/block_queue.h"
#include "utils/common_utils.h"
#include "config.pb.h"

namespace bigoai {

using namespace config;
class Module {
public:
    virtual ~Module() {}

    virtual void init_metrics() { return init_metrics(name()); }


    virtual void init_metrics(std::string instance_name) {
        set_name(instance_name);
        metric_cnt_.reset(new bvar::Adder<uint64_t>("bigoai.module", name() + ".total"));
        metric_err_.reset(new bvar::Adder<uint64_t>("bigoai.module", name() + ".error"));
        metric_latency_.reset(new bvar::LatencyRecorder("bigoai.module", name() + ".latency"));
        metric_que_size_.reset(new bvar::IntRecorder("bigoai.module", name() + ".queue.size"));
    }

    virtual bool init(const ModuleConfig &conf) {
        init_metrics(conf.instance_name().size() > 0 ? conf.instance_name() : name());
        return true;
    }

    virtual void start(BlockingQueue<ContextPtr> &in_que, BlockingQueue<ContextPtr> &out_que) {
        auto th = [&]() {
            while (true) {
                auto ctx = in_que.get();
                if (ctx->quit_signal())
                    return;
                if (ctx->enable_modules().find(name()) == ctx->enable_modules().end()) {
                    VLOG(10) << "[" << name() << "] will be skiped, not found module name in enable_modules field. [ID:"
                             << ctx->traceid() << "]";
                    out_que.put(ctx);
                    continue;
                }
                auto beg = get_time_ms();
                auto ret = do_context(ctx);
                auto cost = get_time_ms() - beg;
                update_metrics(ret, out_que.size(), cost);
                out_que.put(ctx);
            }
        };
        std::vector<std::thread> pool;
        for (int i = 0; i < work_thread_num_; ++i) {
            pool.emplace_back(th);
        }
        for (int i = 0; i < work_thread_num_; ++i) {
            pool[i].join();
        }
        LOG(INFO) << "Module[" << name() << "] quit.";
    }

    virtual bool do_context(ContextPtr &ctx) { return true; }
    virtual std::string name() const { return name_; }
    virtual void set_name(std::string instance_name) { name_ = instance_name; }

    virtual void quit(BlockingQueue<ContextPtr> &in_que) {
        LOG(INFO) << "Recived quit signal. " << name();
        while (in_que.size()) {
            LOG(INFO) << "Waiting for queue empty. Queue size: " << in_que.size() << ", " << name();
            sleep(1);
        }
        for (int i = 0; i < work_thread_num_; ++i) {
            ContextPtr ctx(new Context());
            ctx->set_quit_signal(true);
            in_que.put(ctx);
        }
    }

    virtual void update_metrics(bool success, int que_size, uint64_t latency = 0) {
        *metric_cnt_ << 1;
        *metric_que_size_ << que_size;
        if (success) {
            *metric_latency_ << latency;
        } else {
            *metric_err_ << 1;
        }
    }

protected:
    int work_thread_num_ = 1;
    std::string name_ = "BaseModule";
    std::shared_ptr<bvar::Adder<uint64_t>> metric_cnt_;
    std::shared_ptr<bvar::Adder<uint64_t>> metric_err_;
    std::shared_ptr<bvar::IntRecorder> metric_que_size_;
    std::shared_ptr<bvar::LatencyRecorder> metric_latency_;
};


template<typename ClassName>
class ClassRegister {
public:
    typedef ClassName *(*Constructor)(void);

private:
    typedef std::map<std::string, Constructor> ClassMap;

    ClassMap constructor_map_;

public:
    void AddConstructor(const std::string class_name, Constructor constructor) {
        typename ClassMap::iterator it = constructor_map_.find(class_name);
        if (it != constructor_map_.end()) {
            // std::cout << "error!";
            return;
        }
        constructor_map_[class_name] = constructor;
    }


    ClassName *CreateObject(const std::string class_name) const {
        typename ClassMap::const_iterator it = constructor_map_.find(class_name);
        if (it == constructor_map_.end()) {
            return nullptr;
        }
        return (*(it->second))();
    }
};


// 用来保存每个基类的 ClassRegister static 对象，用于全局调用
template<typename ClassName>
ClassRegister<ClassName> &GetRegister() {
    static ClassRegister<ClassName> class_register;
    return class_register;
}


// 每个类的构造函数，返回对应的base指针
template<typename BaseClassName, typename SubClassName>
BaseClassName *NewObject() {
    return new SubClassName();
}


// 为每个类反射提供一个 helper，构造时就完成反射函数对注册
template<typename BaseClassName>
class ClassRegisterHelper {
public:
    ClassRegisterHelper(const std::string sub_class_name,
                        typename ClassRegister<BaseClassName>::Constructor constructor) {
        GetRegister<BaseClassName>().AddConstructor(sub_class_name, constructor);
    }


    ~ClassRegisterHelper() {}
};


#define RegisterClass(base_class_name, sub_class_name)                            \
    static ClassRegisterHelper<base_class_name> sub_class_name##_register_helper( \
        #sub_class_name, NewObject<base_class_name, sub_class_name>);


#define CreateObject(base_class_name, sub_class_name_as_string) \
    GetRegister<base_class_name>().CreateObject(sub_class_name_as_string)
}
