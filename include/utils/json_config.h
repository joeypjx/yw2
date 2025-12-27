// ============================================================================
// 文件功能描述：
// JSON配置管理器（JsonConfig）的头文件，定义应用程序配置的加载和访问接口。
// 主要功能包括：
// 1. 单例模式：提供全局单例访问，确保全局只有一个配置实例
// 2. 配置文件加载：提供从JSON文件加载配置数据的功能
// 3. 配置访问：提供Get模板方法，支持按点号路径访问嵌套配置（如"db.conninfo"）
// 4. 类型安全：使用模板方法支持多种数据类型（字符串、整数、布尔值等）
// 5. 默认值支持：访问不存在的配置项时返回指定的默认值
// ============================================================================

#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace yw {
namespace utils {

class JsonConfig {
public:
    // 全局单例访问
    static JsonConfig& instance();

    // 全局加载与读取（便于直接在任意位置使用）
    static bool Load(const std::string& path) { return instance().loadFromFile(path); }
    static const nlohmann::json& Data() { return instance().data_; }
    template <typename T>
    static T Get(const std::string& key, const T& def) { return instance().get<T>(key, def); }

    // 从文件加载 JSON（UTF-8），成功返回 true
    bool loadFromFile(const std::string& path);

    // 获取原始 JSON 引用
    const nlohmann::json& data() const { return data_; }

    // 按键读取，若不存在返回默认值
    template <typename T>
    T get(const std::string& key, const T& def) const {
        if (!data_.is_object()) return def;

        // 支持点号路径（例如："db.conninfo" / "scanner.manager_ip"）
        const nlohmann::json* current = &data_;
        std::size_t start = 0;
        while (start <= key.size()) {
            std::size_t dot = key.find('.', start);
            std::string part = key.substr(start, (dot == std::string::npos) ? std::string::npos : (dot - start));
            if (part.empty()) return def;

            auto it = current->find(part);
            if (it == current->end() || it->is_null()) return def;

            if (dot == std::string::npos) {
                // 最后一级，尝试转换为目标类型
                try {
                    return it->get<T>();
                } catch (...) {
                    return def;
                }
            } else {
                // 继续深入
                if (!it->is_object()) return def;
                current = &(*it);
                start = dot + 1;
            }
        }
        return def;
    }

private:
    nlohmann::json data_;
};

} // namespace utils
} // namespace yw


