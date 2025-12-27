// ============================================================================
// 文件功能描述：
// 节点管理器（NodeManager）的头文件，定义节点管理服务的接口和实现。
// 主要功能包括：
// 1. 节点管理：实现INodeModule接口，提供节点查询和管理功能
// 2. 心跳处理：接收节点通过POST /heartbeat发送的心跳数据，更新节点缓存
// 3. 在线状态判断：根据节点最后更新时间判断节点在线/离线状态
// 4. 板卡类型变化检测：检测节点板卡类型变化，触发告警回调
// 5. 节点扫描器：集成MulticastScanner，通过组播方式主动发现网络中的节点
// 6. HTTP路由注册：注册/heartbeat端点，处理节点心跳请求
// ============================================================================

#pragma once

#include "node/node.h"

#include "node_cache.h"
#include <memory>
#include <string>
#include <mutex>
#include <functional>
// 前向声明，避免直接依赖 server 头
namespace hv { struct HttpService; }

namespace yw { namespace utils { class MulticastScanner; } }


namespace yw {
namespace node {

/**
 * @brief 节点管理服务类，使用libhv实现heartbeat HTTP接口
 * 
 * 提供REST API服务，接收节点心跳数据并存储到NodeCache中
 * 支持启动/停止HTTP服务器，处理心跳请求
 */
class NodeManager : public INodeModule {
public:
    /**
     * @brief 构造函数
     * @param server HTTP服务器实例（通过依赖注入）
     */
    explicit NodeManager(std::shared_ptr<hv::HttpService> service);
    
    /**
     * @brief 析构函数
     */
    ~NodeManager();
    // INodeModule 接口实现
    std::vector<NodeExt> getAllNodes() const override;
    std::optional<NodeExt> getNodeByIP(const std::string& ip) const override;
    std::vector<NodeExt> getNodesByBoxId(int box_id) const override;

    /**
     * @brief 设置告警回调函数
     * @param callback 回调函数，参数为：box_id、slot_id、缓存的板卡类型、新的板卡类型
     */
    void setAlertCallback(std::function<void(int, int, const std::string&, const std::string&)> callback) override;

    // 禁止拷贝和赋值
    NodeManager(const NodeManager&) = delete;
    NodeManager& operator=(const NodeManager&) = delete;

private:
    /**
     * @brief 初始化HTTP路由
     */
    void setupRoutes();

private:
    std::shared_ptr<hv::HttpService> service_;  // 共享路由服务（注入）
    std::unique_ptr<NodeCache> node_cache_;     // 节点缓存
    std::unique_ptr<yw::utils::MulticastScanner> scanner_; // 通用组播扫描器
    std::int64_t online_threshold_ms_;          // 节点在线状态判断阈值（毫秒）
    std::function<void(int, int, const std::string&, const std::string&)> alert_callback_; // 告警回调函数
};

} // namespace node
} // namespace yw
