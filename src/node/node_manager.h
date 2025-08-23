#pragma once

#include "yw/node.h"

#include "node_cache.h"
#include <memory>
#include <string>
#include <mutex>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>

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
    explicit NodeManager(std::shared_ptr<hv::HttpServer> server);
    
    /**
     * @brief 析构函数
     */
    ~NodeManager();

    // 禁止拷贝和赋值
    NodeManager(const NodeManager&) = delete;
    NodeManager& operator=(const NodeManager&) = delete;

private:
    /**
     * @brief 初始化HTTP路由
     */
    void setupRoutes();

private:
    std::shared_ptr<hv::HttpServer> server_;    // HTTP服务器实例（注入）
    std::unique_ptr<hv::HttpService> router_;   // HTTP路由实例（自己管理）
    std::unique_ptr<NodeCache> node_cache_;     // 节点缓存
};

} // namespace node
} // namespace yw
