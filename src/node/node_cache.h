#pragma once

#include "yw/node_model.h"
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
