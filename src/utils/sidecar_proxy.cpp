#include <rapidjson/document.h>

#include "utils/common_utils.h"
#include "utils/sidecar_proxy.h"

namespace bigoai {

// 针对通过 proxy sidecar 代理的服务返回结果解析, return json_obj["status"]
// 如果 status != 200, result=json_obj["results"]
// 如果 status == 200, result=json_obj["results"][result_key]
bool parse_proxy_sidecar_response(const std::string &content, const std::string &result_key, std::string &result,
                                  int &status_code) {
    rapidjson::Document json_obj;
    json_obj.Parse(content.c_str());
    if (!(json_obj.HasMember("status") && json_obj["status"].IsInt() && json_obj.HasMember("results") &&
          json_obj["results"].IsObject())) {
        return false;
    }
    status_code = json_obj["status"].GetInt();
    if (json_obj["status"].GetInt() == 200) {
        if (!(json_obj["results"].HasMember(result_key.c_str()) &&
              json_obj["results"][result_key.c_str()].IsObject())) {
            return false;
        }
        result = stringify(json_obj["results"][result_key.c_str()]);
    }
    return true;
}

}
