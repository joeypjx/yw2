#include "monitor_cache.h"

namespace yw {
namespace monitor {

MonitorCache::MonitorCache() = default;
MonitorCache::~MonitorCache() = default;

void MonitorCache::put(const Resource& res, std::int64_t timestamp_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& entry = map_[res.host_ip];
    entry.data = res;
    entry.updated_at_ms = timestamp_ms;
}

std::optional<Resource> MonitorCache::get(const std::string& host_ip) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(host_ip);
    if (it == map_.end()) return std::nullopt;
    return it->second.data;
}

std::int64_t MonitorCache::getUpdatedAt(const std::string& host_ip) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(host_ip);
    if (it == map_.end()) return 0;
    return it->second.updated_at_ms;
}

std::vector<std::string> MonitorCache::getAllHosts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> hosts;
    hosts.reserve(map_.size());
    for (const auto& [ip, _] : map_) hosts.push_back(ip);
    return hosts;
}

} // namespace monitor
} // namespace yw


