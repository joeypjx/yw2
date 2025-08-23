#include "node_cache.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>

namespace yw {
namespace node {

NodeCache::NodeCache() = default;

NodeCache::~NodeCache() = default;

bool NodeCache::addOrUpdateNode(const Node& node) {
    if (node.host_ip.empty()) {
        return false;  // IP地址不能为空
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    // 更新记录（数据 + 元数据）
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();
    NodeRecord& rec = nodes_[node.host_ip];
    rec.node = node;
    rec.last_update_ms = static_cast<std::int64_t>(now_ms);

    spdlog::info("NodeCache::addOrUpdateNode: {}, updated_at_ms={}", node.host_ip, rec.last_update_ms);
    
    return true;
}

std::optional<Node> NodeCache::getNode(const std::string& ip) const {
    if (ip.empty()) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(ip);
    if (it != nodes_.end()) {
        return it->second.node;
    }
    return std::nullopt;
}

std::vector<Node> NodeCache::getAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Node> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(pair.second.node);
    }
    return result;
}

std::optional<std::int64_t> NodeCache::getLastUpdateMs(const std::string& ip) const {
    if (ip.empty()) {
        return std::nullopt;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(ip);
    if (it == nodes_.end()) {
        return std::nullopt;
    }
    return it->second.last_update_ms;
}

} // namespace node
} // namespace yw
