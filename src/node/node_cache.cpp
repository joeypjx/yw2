// ============================================================================
// 文件功能描述：
// 节点缓存（NodeCache）的实现文件，提供节点信息的内存缓存功能。
// 主要功能包括：
// 1. 缓存初始化：为所有机箱和槽位创建初始节点记录（9个机箱，每个12个槽位，跳过槽位6和7）
// 2. 节点更新：提供addOrUpdateNode方法，更新或添加节点信息并记录最后更新时间戳
// 3. 节点查询：提供按IP地址查询节点信息的功能，返回包含最后更新时间的扩展信息
// 4. 批量查询：提供获取所有缓存的节点信息的功能
// 5. IP地址计算：使用IPAddressUtils工具类根据机箱号和槽位号计算节点IP地址
// 6. 线程安全：使用互斥锁保护缓存数据，支持多线程并发访问
// ============================================================================

#include "node_cache.h"
#include "utils/ip_address_utils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <mutex>

namespace yw {
namespace node {

// 节点缓存构造函数，自动初始化所有可能的节点槽位
NodeCache::NodeCache() {
    initialize();
}

NodeCache::~NodeCache() = default;

// 初始化节点缓存，为所有机箱和槽位创建初始节点记录
// 支持9个机箱，每个机箱12个槽位（跳过槽位6和7）
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

// 添加或更新节点信息
// 如果节点已存在则更新，不存在则添加，同时更新最后更新时间戳
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

// 根据IP地址获取节点信息（包含最后更新时间）
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

// 获取所有节点信息（包含最后更新时间）
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
