// ============================================================================
// 文件功能描述：
// 节点缓存（NodeCache）的头文件，定义节点信息内存缓存的接口。
// 主要功能包括：
// 1. 缓存管理：提供节点信息的存储、查询和更新功能
// 2. 初始化：为所有机箱和槽位创建初始节点记录
// 3. 节点更新：提供addOrUpdateNode方法，更新或添加节点信息并记录最后更新时间
// 4. 节点查询：提供按IP地址查询节点信息的功能
// 5. 批量查询：提供获取所有缓存的节点信息的功能
// 6. 线程安全：使用互斥锁保护缓存数据，支持多线程并发访问
// ============================================================================

#pragma once

#include "node/node_model.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <mutex>
#include <cstdint>

namespace yw {
namespace node {

/**
 * @brief 节点缓存类，负责存储和缓存Node对象
 * 
 * 提供基于IP地址的节点存储、查询、更新功能
 * 线程安全，支持并发访问
 */
class NodeCache {
public:

    // 构造函数和析构函数
    NodeCache();
    ~NodeCache();

    // 禁止拷贝和赋值
    NodeCache(const NodeCache&) = delete;
    NodeCache& operator=(const NodeCache&) = delete;

    bool initialize();

    // 基本操作
    /**
     * @brief 添加或更新节点
     * @param node 节点信息
     * @return 成功返回true，失败返回false（如IP为空）
     */
    bool addOrUpdateNode(const Node& node);

    /**
     * @brief 根据IP地址获取节点
     * @param ip IP地址
     * @return 存在返回节点信息，不存在返回nullopt
     */
    std::optional<NodeExt> getNode(const std::string& ip) const;

    // 批量操作
    /**
     * @brief 获取所有节点
     * @return 所有节点的副本
     */
    std::vector<NodeExt> getAllNodes() const;

    // 获取最后更新时间接口已由 NodeExt.updated_at 覆盖，不再单独提供

    // 查询操作（保留必要接口）

private:
    // 缓存记录：包含节点数据与最后更新时间
    struct NodeRecord {
        Node node;
        std::int64_t last_update_ms = 0;
    };

    mutable std::mutex mutex_;                           // 线程安全锁
    std::unordered_map<std::string, NodeRecord> nodes_;  // IP地址到缓存记录的映射
};

} // namespace node
} // namespace yw
