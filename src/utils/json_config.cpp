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


