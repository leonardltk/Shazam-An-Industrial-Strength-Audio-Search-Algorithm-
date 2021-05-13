#include "common_utils.h"
#include <cstdio>

DEFINE_int32(log_max_len, 101, "log response_attachment len");
DEFINE_string(dispatcher_filter_effect_conf, "./conf/filter_effect.json", "Path to config file of effect filtering.");


namespace bigoai {
bvar::LatencyRecorder g_req_latency("log_latency", "log_latency");


LogRAII::LogRAII(const std::string &name, const std::string &traceid) {
    mod_ = name;
    id_ = traceid;
    //    LOG(INFO) << "++++ Begin " << mod_ << " [id: " << id_ << "]";
}


LogRAII::~LogRAII() {
    //  LOG(INFO) << "---- Finish " << mod_ << " [id: " << id_ << "]";
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::string item;
    std::istringstream is(s);
    std::vector<std::string> ret;
    while (std::getline(is, item, delim)) {
        ret.push_back(item);
    }
    return ret;
}


int startswith(const std::string &s, const std::string &sub) { return s.find(sub) == 0 ? 1 : 0; }


static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";


static inline bool is_base64(unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); }


std::string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                ret += base64_chars[char_array_4[i]];
            }

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++) {
            ret += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            ret += '=';
        }
    }
    return ret;
}


void printSpaces(int x) {
    for (int i = 0; i < x; ++i) {
        std::cout << " ";
    }
}


}
