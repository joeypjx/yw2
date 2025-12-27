// ============================================================================
// 文件功能描述：
// JSON配置管理器（JsonConfig）的实现文件，提供应用程序配置的加载和访问。
// 主要功能包括：
// 1. 单例模式：使用单例模式确保全局只有一个配置实例
// 2. 配置文件加载：从JSON文件加载配置数据，支持嵌套的配置结构
// 3. 配置访问：通过Get模板方法访问配置值，支持默认值设置
// 4. 类型安全：使用模板方法支持多种数据类型（字符串、整数、布尔值等）
// 5. 错误处理：配置文件加载失败时返回false，访问不存在的配置项时返回默认值
// ============================================================================

#include "utils/json_config.h"

#include <fstream>

namespace yw {
namespace utils {

// 获取JSON配置单例实例
// 返回: 配置对象引用
JsonConfig& JsonConfig::instance() {
    static JsonConfig cfg;
    return cfg;
}

// 从文件加载JSON配置
// path: 配置文件路径
// 返回: 加载成功返回true，失败返回false
bool JsonConfig::loadFromFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    try {
        nlohmann::json j; ifs >> j; data_ = std::move(j);
        return true;
    } catch (...) {
        data_.clear();
        return false;
    }
}

} // namespace utils
} // namespace yw


