// ============================================================================
// 文件功能描述：
// 节点管理器（NodeManager）的实现文件，负责管理计算节点的注册、心跳和状态跟踪。
// 主要功能包括：
// 1. 节点注册与心跳：接收节点通过POST /heartbeat发送的心跳数据，更新节点信息
// 2. 节点缓存管理：使用NodeCache维护节点信息的内存缓存，提供快速查询
// 3. 在线状态判断：根据节点最后更新时间判断节点在线/离线状态（可配置阈值）
// 4. 节点扫描器：启动MulticastScanner，通过组播方式主动发现网络中的节点
// 5. 板卡类型变化检测：检测节点板卡类型变化，触发告警回调通知告警模块
// 6. 数据验证：验证心跳请求中的节点数据格式和有效性（IP地址、端口范围等）
// 7. 查询接口：提供按IP、按机箱ID查询节点信息的功能
// 8. HTTP路由注册：注册/heartbeat端点，处理节点心跳请求
// ============================================================================

#include "node_manager.h"
#include "node_cache.h"
#include "node/node_model.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include <chrono>
#include "utils/multicast_scanner.h"
#include "utils/json_config.h"
#include <fstream>
#include <regex>
#include <nlohmann/json.hpp>

namespace yw {
namespace node {

// 节点管理器构造函数
// service: HTTP服务实例，用于注册节点相关的API路由
// 初始化节点缓存、启动节点扫描器，并设置路由
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

// 节点管理器析构函数
// HTTP服务器由AppContext管理，无需特殊处理
NodeManager::~NodeManager() {
    // 析构函数不需要特殊处理，HTTP服务器由AppContext管理
}

// INodeModule 接口实现
// 获取所有节点信息，并根据最后更新时间判断在线状态
// 返回: 所有节点的扩展信息列表（包含在线状态）
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

// 根据IP地址获取节点信息
// ip: 节点IP地址
// 返回: 节点扩展信息，不存在时返回std::nullopt
std::optional<NodeExt> NodeManager::getNodeByIP(const std::string& ip) const {
    auto ext = node_cache_->getNode(ip);
    if (!ext) return std::nullopt;
    const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()
    ).time_since_epoch().count();
    ext->status = (now_ms - ext->updated_at) <= online_threshold_ms_ ? "online" : "offline";
    return ext;
}

// 根据机箱号获取节点列表
// box_id: 机箱编号（1-9）
// 返回: 匹配机箱号的所有节点扩展信息列表
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

void NodeManager::setAlertCallback(std::function<void(int, int, const std::string&, const std::string&)> callback) {
    alert_callback_ = std::move(callback);
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
    // nodeJson: 节点数据JSON对象
    // 返回: 验证通过返回空字符串，失败返回错误消息
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

// 设置节点相关的HTTP路由
// 注册 /heartbeat 和 /resource 端点
void NodeManager::setupRoutes() {
    if (!service_) {
        spdlog::error("HttpService not available for route setup");
        return;
    }
    
    // POST /heartbeat - 接收节点心跳数据
    // 请求体：包含节点信息的JSON对象
    // 功能：更新节点缓存，检测板卡类型变化并触发告警
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
            
            auto cached_node = node_cache_->getNode(node.host_ip);
            if (cached_node) {
                // 比较板卡类型
                // 只有当缓存的板卡类型不为空，且新的板卡类型也不为空，且两者不同时，才创建告警
                // 如果缓存的板卡类型为空，说明是第一次设置，不算变化
                if (!cached_node->board_type.empty() && !node.board_type.empty() && cached_node->board_type != node.board_type) {
                    // 告警
                    spdlog::info("板卡类型变化: 节点 {} 的板卡类型从 {} 变为 {}", node.host_ip, cached_node->board_type, node.board_type);
                    // 调用告警回调函数
                    if (alert_callback_) {
                        alert_callback_(node.box_id, node.slot_id, cached_node->board_type, node.board_type);
                    }
                }
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