#include "node_manager.h"
#include "node_cache.h"
#include "node.h"
#include <iostream>
#include <sstream>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>

namespace yw {
namespace node {

NodeManager::NodeManager(std::shared_ptr<hv::HttpServer> server)
    : server_(server) {
    node_cache_ = std::make_unique<NodeCache>();
    router_ = std::make_unique<hv::HttpService>();
    router_->AllowCORS();

    setupRoutes();
}

NodeManager::~NodeManager() {
    // 析构函数不需要特殊处理，HTTP服务器由AppContext管理
}

void NodeManager::setupRoutes() {
    if (!server_) {
        std::cerr << "HTTP server not available for route setup" << std::endl;
        return;
    }
    
    // 创建HttpService并配置路由
    router_->POST("/heartbeat", [this](const HttpContextPtr& ctx) {
        // 处理心跳请求的逻辑
        std::cout << "Heartbeat received" << std::endl;

        // 解析请求体 -> 提取 data 字段并转换为 Node（仅转换，不做其他处理）
        const auto j = nlohmann::json::parse(ctx->body());
        if (j.contains("data")) {
            const Node node = j["data"].get<Node>();
            node_cache_->addOrUpdateNode(node);
        }

        return 200;
    });
    
    // 获取所有节点（包含更新时间）
    router_->GET("/nodes", [this](const HttpContextPtr& ctx) {
        const auto nodes = node_cache_->getAllNodes();
        nlohmann::json resp = nlohmann::json::array();
        for (const auto& n : nodes) {
            const auto ts = node_cache_->getLastUpdateMs(n.host_ip).value_or(0);
            NodeExt ext(n, ts);
            resp.push_back(ext);
        }
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });
    
    // 将HttpService注册到HttpServer
    server_->registerHttpService(router_.get());
    
    std::cout << "NodeManager routes configured and registered to HttpServer" << std::endl;
}

} // namespace node
} // namespace yw