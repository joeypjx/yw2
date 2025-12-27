#include "utils/time_utils.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace yw {
namespace utils {

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

