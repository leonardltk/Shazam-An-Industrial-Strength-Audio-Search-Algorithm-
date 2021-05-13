#pragma once

#include <fstream>
#include <chrono>
#include <butil/logging.h>
#include <brpc/server.h>

DEFINE_bool(vlog, true, "");

namespace bigoai {

bvar::LatencyRecorder g_req_latency("log_latency", "log_latency");

inline uint64_t get_time_ms(void) {
  return std::chrono::duration<double>(
      std::chrono::high_resolution_clock::now().time_since_epoch()).count() * 1000;
}

class LogRAII {
public:
  LogRAII(const brpc::Controller *cntl, std::string msg)
   :cntl_(cntl), msg_(msg) {
    LOG_IF(INFO, FLAGS_vlog) << "++ request"
              << ", from " << cntl_->remote_side() 
              << ", [" << msg_ << "]";
   beg_ms_ = get_time_ms();
  }

  ~LogRAII() {
    auto latency = get_time_ms() - beg_ms_;
    LOG_IF(INFO, FLAGS_vlog) << "-- request"
              << ", from " << cntl_->remote_side() 
              << ", [" << msg_ << "]"
              << ", status: " << cntl_->http_response().status_code()
              << ", response: " << cntl_->response_attachment().to_string().substr(0, 101)
              << ", latency： " << latency << "ms";
    g_req_latency << latency;
  }

private:
  const brpc::Controller *cntl_ = nullptr;  // weak ptr
  std::string msg_;
  uint64_t beg_ms_;
};

inline std::vector<std::string> split(const std::string& s, char delim) {
  std::string item;
  std::istringstream is(s);
  std::vector<std::string> ret;
  while (std::getline(is, item, delim)) {
    ret.push_back(item);
  }
  return ret;
}

inline int make_response(brpc::Controller *cntl, int status_code, const std::string& msg, const std::string& result = "{}"){
  std::ostringstream os;
  os << "{\"status\":" << status_code << ","
     << "\"msg\":\"" << msg << "\","
     << "\"results\":" << result << "}";
  cntl->response_attachment().append(os.str());
  cntl->http_response().set_status_code(status_code);
  return 0;
}

} // namespace bigoai
