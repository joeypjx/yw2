// ============================================================================
// 文件功能描述：
// 监控缓存（MonitorCache）的实现文件，提供资源监控数据的内存缓存功能。
// 主要功能包括：
// 1. 数据缓存：以节点IP地址为键，缓存每个节点的最新资源监控数据
// 2. 数据更新：提供put方法，更新或添加资源数据并记录更新时间戳
// 3. 数据查询：提供按IP地址查询资源数据的功能
// 4. 时间戳查询：提供查询资源数据最后更新时间戳的功能
// 5. 批量查询：提供获取缓存中所有主机IP地址列表的功能
// 6. 线程安全：使用互斥锁保护缓存数据，支持多线程并发访问
// ============================================================================

#include "monitor_cache.h"

namespace yw {
namespace monitor {

// 监控缓存构造函数
MonitorCache::MonitorCache() = default;

// 监控缓存析构函数
MonitorCache::~MonitorCache() = default;

// 将资源数据存入缓存
// res: 资源数据对象（包含host_ip等字段）
// timestamp_ms: 更新时间戳（毫秒）
void MonitorCache::put(const Resource& res, std::int64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& entry = map_[res.host_ip];
    entry.data = res;
    entry.updated_at_ms = timestamp_ms;
}

// 根据主机IP获取缓存的资源数据
// host_ip: 节点IP地址
// 返回: 资源数据对象，不存在时返回std::nullopt
std::optional<Resource> MonitorCache::get(const std::string& host_ip) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(host_ip);
    if (it == map_.end()) return std::nullopt;
    return it->second.data;
}

// 获取资源数据的最后更新时间戳
// host_ip: 节点IP地址
// 返回: 更新时间戳（毫秒），不存在时返回0
std::int64_t MonitorCache::getUpdatedAt(const std::string& host_ip) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(host_ip);
    if (it == map_.end()) return 0;
    return it->second.updated_at_ms;
}

// 获取缓存中所有主机的IP地址列表
// 返回: 所有主机IP地址的向量
std::vector<std::string> MonitorCache::getAllHosts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> hosts;
    hosts.reserve(map_.size());
    for (const auto& [ip, _] : map_) hosts.push_back(ip);
    return hosts;
}

} // namespace monitor
} // namespace yw


