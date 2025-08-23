#include "node_manager.h"
#include "node_cache.h"
#include "yw/node_model.h"
#include <iostream>
#include <sstream>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include <chrono>
#include "yw/MulticastScanner.h"

namespace yw {
namespace node {

NodeManager::NodeManager(std::shared_ptr<hv::HttpService> service)
    : service_(std::move(service)) {
    node_cache_ = std::make_unique<NodeCache>();
    service_->AllowCORS();

    // 启动节点扫描器（示例：以本机IP与HTTP端口启动）
    // TODO: manager_ip 可从配置或探测获取
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        "192.168.10.254",   // manager_ip
        8080,                 // manager_port
        "/heartbeat"         // url
    );
    scanner_->start();

    setupRoutes();
}

NodeManager::~NodeManager() {
    // 析构函数不需要特殊处理，HTTP服务器由AppContext管理
}

// INodeModule 接口实现
std::vector<NodeExt> NodeManager::getAllNodes() const {
    auto list = node_cache_->getAllNodes();
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();
    for (auto& ext : list) {
        const bool is_online = (now_ms - ext.updated_at) <= 10000;
        ext.status = is_online ? "online" : "offline";
    }
    return list;
}

void NodeManager::setupRoutes() {
    if (!service_) {
        std::cerr << "HttpService not available for route setup" << std::endl;
        return;
    }
    
    // 创建HttpService并配置路由
    service_->POST("/heartbeat", [this](const HttpContextPtr& ctx) {
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
    service_->GET("/nodes", [this](const HttpContextPtr& ctx) {
        const auto nodes = node_cache_->getAllNodes();
        nlohmann::json resp = nlohmann::json::array();
        const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now()
        ).time_since_epoch().count();
        for (auto ext : nodes) {
            const bool is_online = (now_ms - ext.updated_at) <= 10000; // 10秒内为online
            ext.status = is_online ? "online" : "offline";
            resp.push_back(ext);
        }
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });
    
    std::cout << "NodeManager routes configured on shared HttpService" << std::endl;
}

} // namespace node
} // namespace yw