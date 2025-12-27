// ============================================================================
// 文件功能描述：
// 持续时间工具类（DurationUtils）的头文件，定义持续时间字符串解析和转换的接口。
// 主要功能包括：
// 1. 持续时间解析：提供解析简写格式持续时间字符串的功能（如"5s"、"10m"、"2h"）
// 2. 单位转换：支持秒（s）、分钟（m）、小时（h）三种时间单位
// 3. 毫秒转换：提供parseToMilliseconds方法，将持续时间转换为毫秒数
// 4. PostgreSQL格式转换：提供parseToPgInterval和parseToPgStandard方法，转换为PostgreSQL格式
// 5. 秒数转换：提供parseToSeconds方法，将持续时间转换为秒数
// ============================================================================

#pragma once

#include <string>
#include <cstdint>

namespace yw {
namespace utils {

/**
 * 时间解析工具类
 * 提供统一的时间字符串解析功能，支持多种输出格式
 */
class DurationUtils {
public:
    /**
     * 解析时间字符串为毫秒数
     * @param duration 时间字符串，支持格式：123s、123m、123h
     * @param default_ms 默认毫秒数，当解析失败时返回
     * @return 毫秒数
     */
    static std::int64_t parseToMilliseconds(const std::string& duration, std::int64_t default_ms = 0);

    /**
     * 解析时间字符串为 PostgreSQL INTERVAL 格式
     * @param duration 时间字符串，支持格式：123s、123m、123h
     * @param default_interval 默认间隔，当解析失败时返回
     * @return PostgreSQL INTERVAL 格式字符串，如 "INTERVAL '5 minutes'"
     */
    static std::string parseToPgInterval(const std::string& duration, const std::string& default_interval = "INTERVAL '1 minutes'");

    /**
     * 解析时间字符串为 PostgreSQL 标准格式
     * @param duration 时间字符串，支持格式：123s、123m、123h
     * @param default_interval 默认间隔，当解析失败时返回
     * @return PostgreSQL 标准格式字符串，如 "5 minutes"
     */
    static std::string parseToPgStandard(const std::string& duration, const std::string& default_interval = "1 minute");

    /**
     * 解析时间字符串为秒数
     * @param duration 时间字符串，支持格式：123s、123m、123h
     * @param default_seconds 默认秒数，当解析失败时返回
     * @return 秒数
     */
    static int parseToSeconds(const std::string& duration, int default_seconds = 0);

private:
    /**
     * 内部解析函数，提取数值和单位
     * @param duration 时间字符串
     * @param value 输出参数：解析出的数值
     * @param unit 输出参数：解析出的单位字符
     * @return 是否解析成功
     */
    static bool parseDuration(const std::string& duration, std::int64_t& value, char& unit);
};

} // namespace utils
} // namespace yw
