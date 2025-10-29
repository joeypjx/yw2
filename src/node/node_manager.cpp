#include "node_manager.h"
#include "node_cache.h"
#include "yw/node_model.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include <chrono>
#include "yw/MulticastScanner.h"
#include "yw/JsonConfig.h"
#include <fstream>

namespace yw {
namespace node {

NodeManager::NodeManager(std::shared_ptr<hv::HttpService> service)
    : service_(std::move(service)) {
    node_cache_ = std::make_unique<NodeCache>();
    service_->AllowCORS();

    // 启动节点扫描器（参数改为从配置读取）
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        yw::utils::JsonConfig::Get<std::string>("scanner.manager_ip",
            yw::utils::JsonConfig::Get<std::string>("host", "0.0.0.0")),
        yw::utils::JsonConfig::Get<int>("scanner.manager_port",
            yw::utils::JsonConfig::Get<int>("port", 18888)),
        yw::utils::JsonConfig::Get<std::string>("scanner.url_heartbeat", "/heartbeat"),
        yw::utils::JsonConfig::Get<std::string>("scanner.multicast_ip", "239.192.168.80"),
        yw::utils::JsonConfig::Get<int>("scanner.multicast_port", 3980),
        yw::utils::JsonConfig::Get<int>("scanner.interval_ms", 3000)
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

std::optional<NodeExt> NodeManager::getNodeByIP(const std::string& ip) const {
    auto ext = node_cache_->getNode(ip);
    if (!ext) return std::nullopt;
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();
    ext->status = (now_ms - ext->updated_at) <= 10000 ? "online" : "offline";
    return ext;
}

std::vector<NodeExt> NodeManager::getNodesByBoxId(int box_id) const {
    auto list = node_cache_->getAllNodes();
    std::vector<NodeExt> filtered;
    filtered.reserve(list.size());

    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();

    for (auto& ext : list) {
        if (ext.box_id != box_id) continue;
        const bool is_online = (now_ms - ext.updated_at) <= 10000;
        ext.status = is_online ? "online" : "offline";
        filtered.push_back(std::move(ext));
    }

    return filtered;
}

void NodeManager::setupRoutes() {
    if (!service_) {
        spdlog::error("HttpService not available for route setup");
        return;
    }
    
    // 创建HttpService并配置路由
    service_->POST("/heartbeat", [this](const HttpContextPtr& ctx) {

        // 解析请求体 -> 提取 data 字段并转换为 Node（仅转换，不做其他处理）
        const auto j = nlohmann::json::parse(ctx->body());
        // save to file
        std::ofstream ofs("heartbeat.json");
        ofs << j.dump();
        ofs.close();

        if (j.contains("data")) {
            const Node node = j["data"].get<Node>();
            node_cache_->addOrUpdateNode(node);
        }

        return 200;
    });    
}

} // namespace node
} // namespace yw