#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <cstdint>
#include <vector>

#include "monitor/monitor_model.h"

namespace yw {
namespace monitor {

/**
 * @brief 资源数据缓存：按 host_ip 缓存最近一次上报的 Resource
 * 线程安全
 */
class MonitorCache {
public:
    MonitorCache();
    ~MonitorCache();

    MonitorCache(const MonitorCache&) = delete;
    MonitorCache& operator=(const MonitorCache&) = delete;

    // 写入或更新缓存（覆盖 host_ip 对应的最新值）
    void put(const Resource& res, std::int64_t timestamp_ms);

    // 根据 host_ip 读取最近一次 Resource
    std::optional<Resource> get(const std::string& host_ip) const;

    // 返回 host_ip 对应的最近更新时间（毫秒），不存在返回 0
    std::int64_t getUpdatedAt(const std::string& host_ip) const;

    // 返回当前缓存中的所有 host_ip
    std::vector<std::string> getAllHosts() const;

private:
    struct Entry {
        Resource      data;
        std::int64_t  updated_at_ms = 0;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Entry> map_;
};

} // namespace monitor
} // namespace yw


