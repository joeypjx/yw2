#pragma once

#include <string>
#include <chrono>

namespace yw {
namespace utils {

/**
 * 时间解析工具类
 * 提供统一的时间字符串解析功能
 */
class TimeUtils {
public:
    /**
     * 解析 ISO 时间字符串为 time_point
     * @param isoTime ISO 时间字符串，支持格式：
     *   - 2024-01-01T12:00:00
     *   - 2024-01-01T12:00:00.000
     *   - 2024-01-01T12:00:00Z
     *   - 2024-01-01 12:00:00
     * @return time_point，解析失败返回默认构造的 time_point
     */
    static std::chrono::system_clock::time_point parseISOTime(const std::string& isoTime);
};

} // namespace utils
} // namespace yw

