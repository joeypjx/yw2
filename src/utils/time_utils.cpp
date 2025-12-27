// ============================================================================
// 文件功能描述：
// 时间工具类（TimeUtils）的实现文件，提供时间字符串解析和格式化功能。
// 主要功能包括：
// 1. ISO 8601时间解析：解析ISO 8601格式的时间字符串（如"2024-01-01T12:00:00.000"）
// 2. 多种格式支持：支持带T分隔符和空格分隔符的时间格式
// 3. 时区处理：支持Z后缀（UTC时间）的处理，转换为本地时间
// 4. 毫秒精度：支持解析和转换毫秒级精度的时间戳
// 5. 错误处理：解析失败时返回默认构造的time_point，并记录错误日志
// ============================================================================

#include "utils/time_utils.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace yw {
namespace utils {

// 解析ISO 8601格式的时间字符串
// isoTime: ISO格式时间字符串（如"2024-01-01T12:00:00.000"或"2024-01-01T12:00:00Z"）
// 返回: 解析后的时间点，失败时返回默认构造的time_point
std::chrono::system_clock::time_point TimeUtils::parseISOTime(const std::string& isoTime) {
    try {        
        // 解析时间字符串 (例如: 2024-01-01T12:00:00.000)
        std::tm tm = {};
        std::istringstream ss(isoTime);
        
        // 处理时间字符串，移除Z后缀（如果存在）
        std::string timeStr = isoTime;
        if (timeStr.back() == 'Z') {
            timeStr.pop_back();
        }
                
        // 解析时间（支持毫秒）
        ss.str(timeStr);
        
        // 尝试解析不同的时间格式
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            // 如果T格式失败，尝试空格格式
            ss.clear();
            ss.str(timeStr);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) {
                spdlog::error("解析时间失败，格式错误: {} (处理后: {})", isoTime, timeStr);
                return std::chrono::system_clock::time_point{};
            }
        }
        
        // 处理毫秒部分（如果存在）
        int milliseconds = 0;
        if (ss.peek() == '.') {
            ss.ignore(); // 跳过 '.'
            ss >> milliseconds;
            if (ss.fail()) {
                milliseconds = 0;
            }
        }
        
        // 转换为time_point（使用本地时间）
        auto time_t = std::mktime(&tm);
        if (time_t == -1) {
            spdlog::error("解析时间失败，mktime返回-1: {}", isoTime);
            return std::chrono::system_clock::time_point{};
        }
                
        auto result = std::chrono::system_clock::from_time_t(time_t);
        
        // 添加毫秒
        result += std::chrono::milliseconds(milliseconds);
        
        return result;
        
    } catch (const std::exception& e) {
        spdlog::error("解析时间失败: {} - {}", isoTime, e.what());
        return std::chrono::system_clock::time_point{};
    }
}

} // namespace utils
} // namespace yw

