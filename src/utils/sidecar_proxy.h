#pragma once

#include <string>

namespace bigoai {
// 针对通过 proxy sidecar 代理的服务返回结果解析, return json_obj["status"]
// 如果 status != 200, result=json_obj["results"]
// 如果 status == 200, result=json_obj["results"][result_key]
bool parse_proxy_sidecar_response(const std::string &content, const std::string &result_key, std::string &result,
                                  int &status_code);
}
