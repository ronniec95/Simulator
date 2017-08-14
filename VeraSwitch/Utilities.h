#pragma once
#include <chrono>
#include <codecvt>
#include <locale>
#include <spdlog\spdlog.h>
#include <sstream>
#include <string>
#include <vector>

inline std::wstring s2ws(const std::string &str) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

inline std::string ws2s(const std::wstring &wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

template <typename Out> void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) { *(result++) = item; }
}
template <typename Out> auto split(const std::string &s, char delim) -> Out {
    Out result;
    for (auto i = 0UL, start = 0UL; i < s.size(); i++) {
        if (s[i] == delim) {
            // Using emplace to directly construct the string 'in place', avoiding a temporary (string mem alloc
            // happening here :( )
            result.emplace_back(begin(s) + start, begin(s) + i);
            start = i + 1;
        }
    }
    return result;
}

inline auto split_iter(const std::string &s, char delim)
    -> std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>> {
    auto result = std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>>();
    for (auto i = 0UL, start = 0UL; i < s.size(); i++) {
        if (s[i] == delim) {
            // Using emplace to directly construct the string 'in place', avoiding a temporary (string mem alloc
            // happening here :( )
            result.emplace_back(make_pair(begin(s) + start, begin(s) + i));
            start = i + 1;
        }
    }
    return result;
}

// Only used in tracing mode to figure out our call stack
class MethodLogger {
  public:
    MethodLogger(const std::string method) : method_(std::move(method)) {
        SPDLOG_TRACE(logger_, "Entering[{}]", method_);
    }
    ~MethodLogger() { SPDLOG_TRACE(logger_, "Exiting[{}]", method_); }
    auto logger() const { return logger_; }

  private:
    const std::string               method_;
    std::shared_ptr<spdlog::logger> logger_ = spdlog::get("logger");
};