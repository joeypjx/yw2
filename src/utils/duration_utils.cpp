#include "utils/duration_utils.h"
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

namespace yw {
namespace utils {

bool DurationUtils::parseDuration(const std::string& duration, std::int64_t& value, char& unit) {
    if (duration.empty()) {
        return false;
    }

    // 去除前后空白
    std::string trimmed = duration;
    auto not_space = [](int ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), not_space));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), not_space).base(), trimmed.end());

    if (trimmed.empty()) {
        return false;
    }

    // 检查最后一个字符是否为字母（单位）
    char last_char = trimmed.back();
    if (std::isalpha(static_cast<unsigned char>(last_char))) {
        unit = last_char;
        trimmed.pop_back(); // 移除单位字符
    } else {
        unit = 's'; // 默认为秒
    }

    // 解析数值部分
    try {
        value = std::stoll(trimmed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::int64_t DurationUtils::parseToMilliseconds(const std::string& duration, std::int64_t default_ms) {
    std::int64_t value;
    char unit;
    
    if (!parseDuration(duration, value, unit)) {
        return default_ms;
    }

    switch (unit) {
        case 's': return value * 1000;
        case 'm': return value * 60 * 1000;
        case 'h': return value * 60 * 60 * 1000;
        default:  return default_ms;
    }
}

std::string DurationUtils::parseToPgInterval(const std::string& duration, const std::string& default_interval) {
    std::int64_t value;
    char unit;
    
    if (!parseDuration(duration, value, unit)) {
        return default_interval;
    }

    std::string unit_str;
    switch (unit) {
        case 's': unit_str = "seconds"; break;
        case 'm': unit_str = "minutes"; break;
        case 'h': unit_str = "hours"; break;
        default:  return default_interval;
    }

    return "INTERVAL '" + std::to_string(value) + " " + unit_str + "'";
}

std::string DurationUtils::parseToPgStandard(const std::string& duration, const std::string& default_interval) {
    std::int64_t value;
    char unit;
    
    if (!parseDuration(duration, value, unit)) {
        return default_interval;
    }

    std::string unit_str;
    switch (unit) {
        case 's': unit_str = "seconds"; break;
        case 'm': unit_str = "minutes"; break;
        case 'h': unit_str = "hours"; break;
        default:  return default_interval;
    }

    return std::to_string(value) + " " + unit_str;
}

int DurationUtils::parseToSeconds(const std::string& duration, int default_seconds) {
    std::int64_t value;
    char unit;
    
    if (!parseDuration(duration, value, unit)) {
        return default_seconds;
    }

    switch (unit) {
        case 's': return static_cast<int>(value);
        case 'm': return static_cast<int>(value * 60);
        case 'h': return static_cast<int>(value * 3600);
        default:  return default_seconds;
    }
}

} // namespace utils
} // namespace yw
