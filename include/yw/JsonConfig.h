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
        auto it = data_.find(key);
        if (it == data_.end() || it->is_null()) return def;
        try { return it->get<T>(); } catch (...) { return def; }
    }

private:
    nlohmann::json data_;
};

} // namespace utils
} // namespace yw


