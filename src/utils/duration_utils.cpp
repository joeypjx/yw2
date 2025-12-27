#include "utils/duration_utils.h"
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

namespace yw {
namespace utils {

// 解析持续时间字符串（如"5s"、"10m"、"2h"）
// duration: 持续时间字符串，格式为"数字+单位"（单位可选，默认为秒）
// value: 输出参数，解析出的数值
// unit: 输出参数，解析出的单位字符（'s'/'m'/'h'）
// 返回: 解析成功返回true，失败返回false
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

// 将持续时间字符串转换为毫秒数
// duration: 持续时间字符串（如"5s"、"10m"、"2h"）
// default_ms: 解析失败时的默认返回值
// 返回: 转换后的毫秒数
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

// 将持续时间字符串转换为PostgreSQL INTERVAL格式（如"INTERVAL '5 seconds'"）
// duration: 持续时间字符串（如"5s"、"10m"、"2h"）
// default_interval: 解析失败时的默认返回值
// 返回: PostgreSQL INTERVAL格式字符串
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

// 将持续时间字符串转换为PostgreSQL标准时间格式（如"5 seconds"）
// duration: 持续时间字符串（如"5s"、"10m"、"2h"）
// default_interval: 解析失败时的默认返回值
// 返回: PostgreSQL标准时间格式字符串
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

// 将持续时间字符串转换为秒数
// duration: 持续时间字符串（如"5s"、"10m"、"2h"）
// default_seconds: 解析失败时的默认返回值
// 返回: 转换后的秒数
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
