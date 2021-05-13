#pragma once

#include <locale>
#include <codecvt>
#include <brpc/server.h>
#include <butil/logging.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <chrono>
#include <fstream>
#include <string>
#include "util.pb.h"

namespace bigoai {
// Recording start
#define TIMER_START(_X) auto _X##_start = std::chrono::steady_clock::now(), _X##_stop = _X##_start

// Recording end
#define TIMER_STOP(_X) _X##_stop = std::chrono::steady_clock::now()

// return duration time with nanosecond
#define TIMER_NSEC(_X) std::chrono::duration_cast<std::chrono::nanoseconds>(_X##_stop - _X##_start).count()

// return duration time with microsecond
#define TIMER_USEC(_X) std::chrono::duration_cast<std::chrono::microseconds>(_X##_stop - _X##_start).count()

// return duration time with millisecond
#define TIMER_MSEC(_X) (0.000001 * std::chrono::duration_cast<std::chrono::nanoseconds>(_X##_stop - _X##_start).count())


inline uint64_t get_time_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline uint64_t get_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline uint64_t get_time_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline void unixTime2Str(uint64_t timestamp, std::string &ret) {
    char date[100];
    struct tm tm = *localtime((time_t *)&timestamp);
    strftime((char *)date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);
    ret = date;
}


class LogRAII {
public:
    LogRAII(const std::string &name, const std::string &traceid);

    ~LogRAII();

private:
    std::string mod_;
    std::string id_;
};


std::vector<std::string> split(const std::string &s, char delim);


int startswith(const std::string &s, const std::string &sub);


std::string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len);


inline std::string urlEncode(const std::string &str) {
    std::string res;
    brpc::PercentEncode(str, &res);
    return res;
}


template<typename T>
std::string stringify(const T &o) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    o.Accept(writer);
    return sb.GetString();
}


void printSpaces(int x);


void printJson(const rapidjson::Value &obj, int pre_spaces = 0);


std::string joinJson(const std::string &json1, const std::string &json2);


bool read_file(const std::string &file_path, std::string &data);


bool check_duet(const Context &ctx);

/* string util */
inline std::string ws2str(const std::wstring &string_to_convert) {
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::string converted_str = converter.to_bytes(string_to_convert);
    return converted_str;
}

inline std::wstring str2ws(const std::string &string_to_convert) {
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::wstring converted_str = converter.from_bytes(string_to_convert);
    return converted_str;
}

}