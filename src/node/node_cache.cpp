#include "node_cache.h"
#include "utils/ip_address_utils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <mutex>

namespace yw {
namespace node {

NodeCache::NodeCache() {
    initialize();
}

NodeCache::~NodeCache() = default;

bool NodeCache::initialize() {
    // init 9 box , 12 boards per box
    std::vector<NodeRecord> nodes;
    for (int box_id = 1; box_id <= 9; box_id++) {
        for (int board_id = 1; board_id <= 12; board_id++) {

            if (board_id == 6 || board_id == 7) {
                continue;
            }

            Node node;
            // 只设置机箱号、槽位号和IP地址，其他字段初始化为空
            node.box_id = box_id;
            node.slot_id = board_id;
            node.cpu_id = 0;
            node.srio_id = 0;
            node.hostname = "";
            node.service_port = 0;
            node.box_type = "";
            node.board_type = "";
            node.cpu_type = "";
            node.os_type = "";
            node.resource_type = "";
            node.cpu_arch = "";
            node.gpu.clear();
            node.manufacturer = "";
            node.serial_number = "";
            node.production_date = "";

            // 使用 IPAddressUtils 计算 IP 地址
            node.host_ip = yw::utils::IPAddressUtils::calculateHostIP(box_id, board_id);
            
            // 如果计算出的 IP 为空，说明参数无效，跳过
            if (node.host_ip.empty()) {
                continue;
            }

            NodeRecord record;
            record.node = node;
            record.last_update_ms = 0;
            
            nodes_.insert(std::make_pair(node.host_ip, record));
        }
    }

    return true;
}

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
    
    return true;
}

std::optional<NodeExt> NodeCache::getNode(const std::string& ip) const {
    if (ip.empty()) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(ip);
    if (it != nodes_.end()) {
        const auto& rec = it->second;
        return NodeExt(rec.node, rec.last_update_ms);
    }
    return std::nullopt;
}

std::vector<NodeExt> NodeCache::getAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NodeExt> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        const auto& rec = pair.second;
        result.emplace_back(rec.node, rec.last_update_ms);
    }
    return result;
}

} // namespace node
} // namespace yw
