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
#include <regex>
#include <nlohmann/json.hpp>

namespace yw {
namespace node {

NodeManager::NodeManager(std::shared_ptr<hv::HttpService> service)
    : service_(std::move(service)) {
    node_cache_ = std::make_unique<NodeCache>();
    service_->AllowCORS();

    // 从配置文件读取节点在线状态判断阈值（默认10秒）
    online_threshold_ms_ = yw::utils::JsonConfig::Get<int>("node.online_threshold_ms", 10000);

    // 启动节点扫描器（使用 node 模块专用配置）
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        yw::utils::JsonConfig::Get<std::string>("node.scanner.manager_ip", "0.0.0.0"),
        yw::utils::JsonConfig::Get<int>("node.scanner.manager_port", 18888),
        yw::utils::JsonConfig::Get<std::string>("node.scanner.url_heartbeat", "/heartbeat"),
        yw::utils::JsonConfig::Get<std::string>("node.scanner.multicast_ip", "239.192.168.80"),
        yw::utils::JsonConfig::Get<int>("node.scanner.multicast_port", 3980),
        yw::utils::JsonConfig::Get<int>("node.scanner.interval_ms", 3000)
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
        const bool is_online = (now_ms - ext.updated_at) <= online_threshold_ms_;
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
    ext->status = (now_ms - ext->updated_at) <= online_threshold_ms_ ? "online" : "offline";
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
        const bool is_online = (now_ms - ext.updated_at) <= online_threshold_ms_;
        ext.status = is_online ? "online" : "offline";
        filtered.push_back(std::move(ext));
    }

    return filtered;
}

namespace {
    // 辅助函数：验证 IP 地址格式
    bool isValidIpAddress(const std::string& ip) {
        // 简单的 IPv4 地址格式验证
        std::regex ipRegex(R"(^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$)");
        std::smatch match;
        if (!std::regex_match(ip, match, ipRegex)) {
            return false;
        }
        
        // 验证每个段是否在 0-255 范围内
        for (int i = 1; i <= 4; ++i) {
            int segment = std::stoi(match[i].str());
            if (segment < 0 || segment > 255) {
                return false;
            }
        }
        return true;
    }
    
    // 辅助函数：验证 Node 数据
    std::string validateNodeData(const nlohmann::json& nodeJson) {
        // 检查必需字段 host_ip
        if (!nodeJson.contains("host_ip") || nodeJson["host_ip"].is_null()) {
            return "host_ip is required";
        }
        
        std::string hostIp = nodeJson["host_ip"].get<std::string>();
        if (hostIp.empty()) {
            return "host_ip cannot be empty";
        }
        
        // 验证 IP 地址格式
        if (!isValidIpAddress(hostIp)) {
            return "invalid host_ip format: " + hostIp;
        }
        
        // 验证端口范围（如果存在）
        if (nodeJson.contains("service_port") && !nodeJson["service_port"].is_null()) {
            int port = nodeJson["service_port"].get<int>();
            if (port < 0 || port > 65535) {
                return "service_port must be between 0 and 65535, got: " + std::to_string(port);
            }
        }
        
        // 验证数值字段范围（如果存在）
        if (nodeJson.contains("box_id") && !nodeJson["box_id"].is_null()) {
            int boxId = nodeJson["box_id"].get<int>();
            if (boxId < 0) {
                return "box_id must be non-negative, got: " + std::to_string(boxId);
            }
        }
        
        if (nodeJson.contains("slot_id") && !nodeJson["slot_id"].is_null()) {
            int slotId = nodeJson["slot_id"].get<int>();
            if (slotId < 0) {
                return "slot_id must be non-negative, got: " + std::to_string(slotId);
            }
        }
        
        // 验证 GPU 数组（如果存在）
        if (nodeJson.contains("gpu") && nodeJson["gpu"].is_array()) {
            for (const auto& gpu : nodeJson["gpu"]) {
                if (gpu.contains("index") && !gpu["index"].is_null()) {
                    int index = gpu["index"].get<int>();
                    if (index < 0) {
                        return "gpu index must be non-negative, got: " + std::to_string(index);
                    }
                }
            }
        }
        
        return ""; // 验证通过
    }
}

void NodeManager::setupRoutes() {
    if (!service_) {
        spdlog::error("HttpService not available for route setup");
        return;
    }
    
    // 创建HttpService并配置路由
    service_->POST("/heartbeat", [this](const HttpContextPtr& ctx) {
        try {
            auto body = ctx->body();
            
            // 检查请求体是否为空
            if (body.empty()) {
                spdlog::warn("Received empty heartbeat request body");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "empty request body"},
                    {"data", nlohmann::json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 解析 JSON
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(body);
            } catch (const nlohmann::json::exception& e) {
                spdlog::warn("Invalid JSON format in heartbeat request: {}", e.what());
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "invalid JSON format: " + std::string(e.what())},
                    {"data", nlohmann::json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 检查是否包含 data 字段
            if (!j.contains("data") || j["data"].is_null()) {
                spdlog::warn("Missing 'data' field in heartbeat request");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "missing 'data' field"},
                    {"data", nlohmann::json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 验证 data 字段中的数据
            std::string validationError = validateNodeData(j["data"]);
            if (!validationError.empty()) {
                spdlog::warn("Node data validation failed: {}", validationError);
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "validation failed: " + validationError},
                    {"data", nlohmann::json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 尝试转换为 Node 对象
            Node node;
            try {
                node = j["data"].get<Node>();
            } catch (const nlohmann::json::exception& e) {
                spdlog::error("Failed to parse Node from JSON: {}", e.what());
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                nlohmann::json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "failed to parse node data: " + std::string(e.what())},
                    {"data", nlohmann::json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 保存到缓存
            node_cache_->addOrUpdateNode(node);
            
            // 返回成功响应
            ctx->setContentType("application/json");
            nlohmann::json resp = {
                {"api_version", 1},
                {"status", "success"},
                {"message", "heartbeat received"},
                {"data", nlohmann::json::object()}
            };
            return ctx->send(resp.dump(2));
            
        } catch (const std::exception& e) {
            spdlog::error("Unexpected error in heartbeat handler: {}", e.what());
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            ctx->setContentType("application/json");
            nlohmann::json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "internal server error: " + std::string(e.what())},
                {"data", nlohmann::json::object()}
            };
            return ctx->send(resp.dump(2));
        }
    });    
}

} // namespace node
} // namespace yw