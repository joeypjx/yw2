#pragma once

#include "yw/node.h"

#include "node_cache.h"
#include <memory>
#include <string>
#include <mutex>
// 前向声明，避免直接依赖 server 头
namespace hv { class HttpService; }

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
};

} // namespace node
} // namespace yw
