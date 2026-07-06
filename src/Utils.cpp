#include "../include/Utils.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace stockflow::utils {

std::string trim(const std::string& value) {
    auto first = std::find_if_not(value.begin(), value.end(),
                                  [](unsigned char c) { return std::isspace(c); });
    auto last = std::find_if_not(value.rbegin(), value.rend(),
                                 [](unsigned char c) { return std::isspace(c); }).base();
    return first < last ? std::string(first, last) : std::string{};
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<std::string> splitCommand(const std::string& line) {
    std::vector<std::string> result;
    std::string token;
    bool quoted = false;
    for (char c : line) {
        if (c == '"') {
            quoted = !quoted;
        } else if (std::isspace(static_cast<unsigned char>(c)) && !quoted) {
            if (!token.empty()) {
                result.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) result.push_back(token);
    return result;
}

static std::string formatTime(const char* format) {
    const std::time_t time = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::tm local{};
#ifdef _MSC_VER
    localtime_s(&local, &time);
#else
    const std::tm* converted = std::localtime(&time);
    if (converted) local = *converted;
#endif
    std::ostringstream output;
    output << std::put_time(&local, format);
    return output.str();
}

std::string now() { return formatTime("%Y-%m-%d %H:%M:%S"); }
std::string today() { return formatTime("%Y-%m-%d"); }

std::string escapeField(const std::string& value) {
    std::string result;
    for (char c : value) {
        if (c == '\\') result += "\\\\";
        else if (c == '\t') result += "\\t";
        else if (c == '\n') result += "\\n";
        else result += c;
    }
    return result;
}

std::string unescapeField(const std::string& value) {
    std::string result;
    bool escaped = false;
    for (char c : value) {
        if (escaped) {
            result += c == 't' ? '\t' : c == 'n' ? '\n' : c;
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            result += c;
        }
    }
    if (escaped) result += '\\';
    return result;
}

std::vector<std::string> splitFields(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool escaped = false;
    for (char c : line) {
        if (c == '\t' && !escaped) {
            result.push_back(unescapeField(field));
            field.clear();
        } else {
            field += c;
            if (escaped) escaped = false;
            else escaped = c == '\\';
        }
    }
    result.push_back(unescapeField(field));
    return result;
}

bool parseInt(const std::string& value, int& result) {
    try {
        std::size_t consumed = 0;
        result = std::stoi(trim(value), &consumed);
        return consumed == trim(value).size();
    } catch (...) { return false; }
}

bool parseDouble(const std::string& value, double& result) {
    try {
        std::size_t consumed = 0;
        result = std::stod(trim(value), &consumed);
        return consumed == trim(value).size();
    } catch (...) { return false; }
}

std::string formatMoney(double value, const std::string& currency) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << value << ' ' << currency;
    return output.str();
}

}
