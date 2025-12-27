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
    // res: 资源数据对象
    // timestamp_ms: 时间戳（毫秒）
    void put(const Resource& res, std::int64_t timestamp_ms);

    // 根据 host_ip 读取最近一次 Resource
    // host_ip: 节点IP地址
    // 返回: 资源数据对象，不存在时返回std::nullopt
    std::optional<Resource> get(const std::string& host_ip) const;

    // 返回 host_ip 对应的最近更新时间（毫秒），不存在返回 0
    // host_ip: 节点IP地址
    // 返回: 最近更新时间戳（毫秒），不存在返回0
    std::int64_t getUpdatedAt(const std::string& host_ip) const;

    // 返回当前缓存中的所有 host_ip
    // 返回: 所有已缓存节点的IP地址列表
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


