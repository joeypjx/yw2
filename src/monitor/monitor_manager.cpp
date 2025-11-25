#include "monitor_manager.h"
#include "yw/monitor_model.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <set>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include "yw/MulticastScanner.h"
#include "yw/node.h"
#include "yw/JsonConfig.h"
#include "yw/DurationUtils.h"
#include <regex>

namespace yw {
namespace monitor {

using json = nlohmann::json;

MonitorManager::MonitorManager(std::shared_ptr<hv::HttpService> service,
                               std::shared_ptr<node::INodeModule> node_module)
    : service_(std::move(service)), node_module_(std::move(node_module)) {
    service_->AllowCORS();
    
    // 启动资源扫描器（使用 monitor.scanner 配置）
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        yw::utils::JsonConfig::Get<std::string>("monitor.scanner.manager_ip", "0.0.0.0"),
        yw::utils::JsonConfig::Get<int>("monitor.scanner.manager_port", 18888),
        yw::utils::JsonConfig::Get<std::string>("monitor.scanner.url_resource", "/resource"),
        yw::utils::JsonConfig::Get<std::string>("monitor.scanner.multicast_ip", "239.192.168.80"),
        yw::utils::JsonConfig::Get<int>("monitor.scanner.multicast_port", 3980),
        yw::utils::JsonConfig::Get<int>("monitor.scanner.interval_ms", 5000)
    );
    scanner_->start();

    // 初始化仓库（连接串改为配置项）
    repository_ = std::make_unique<ResourceRepository>(
        yw::utils::JsonConfig::Get<std::string>("db.conninfo", "postgres://postgres:HZ715Net@localhost:5432/yw")
    );

    // 初始化资源缓存
    monitor_cache_ = std::make_unique<MonitorCache>();

    setupRoutes();
}

MonitorManager::~MonitorManager() = default;

std::shared_ptr<Resource> MonitorManager::getNodeResource(const std::string& host_ip) const {
    if (!monitor_cache_) return nullptr;
    auto opt = monitor_cache_->get(host_ip);
    if (!opt) return nullptr;
    return std::make_shared<Resource>(*opt);
}

MetricsSeries MonitorManager::queryMetricsSeries(const std::string& host_ip,
                                                 const std::string& duration,
                                                 const std::vector<std::string>& kinds) const {
    MetricsSeries empty;
    if (!repository_) return empty;

    std::string intervalStr = yw::utils::DurationUtils::parseToPgStandard(duration, "1 minute");

    return repository_->queryMetricsSeries(host_ip, intervalStr, kinds);
}

ExportData MonitorManager::exportNodeData(const std::string& host_ip,
                                           std::int64_t start_time,
                                           std::int64_t end_time,
                                           const std::vector<std::string>& types) const {
    ExportData result;
    result.ip = host_ip;
    
    // 转换时间戳为字符串格式
    auto formatTimestamp = [](std::int64_t timestamp) -> std::string {
        auto time_point = std::chrono::system_clock::from_time_t(timestamp);
        auto time_t = std::chrono::system_clock::to_time_t(time_point);
        std::tm* tm = std::localtime(&time_t);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
        return std::string(buffer);
    };
    
    result.start_time = formatTimestamp(start_time);
    result.end_time = formatTimestamp(end_time);
    
    if (!repository_) {
        return result;
    }
    
    // 查询原始数据
    MetricsSeries metrics = repository_->queryMetricsSeries(host_ip, start_time, end_time, types);
    
    // 构建类型列表
    std::set<std::string> typeSet;
    if (types.empty()) {
        // 如果未指定类型，则包含所有可能的类型
        typeSet.insert("cpu_usage_percent");
        typeSet.insert("memory_usage_percent");
        // 添加磁盘类型
        for (const auto& [device, points] : metrics.disk) {
            if (!points.empty()) {
                typeSet.insert("disk_" + points[0].mount_point + "_usage_percent");
            }
        }
        // 添加网络类型
        for (const auto& [interface, points] : metrics.network) {
            if (!points.empty()) {
                typeSet.insert("network_" + interface + "_rx_rate");
                typeSet.insert("network_" + interface + "_tx_rate");
            }
        }
        // 添加GPU类型
        for (const auto& [gpu_key, points] : metrics.gpu) {
            if (!points.empty()) {
                typeSet.insert("gpu_" + std::to_string(points[0].index) + "_compute_usage");
                typeSet.insert("gpu_" + std::to_string(points[0].index) + "_mem_usage");
            }
        }
    } else {
        // 使用指定的类型
        typeSet.insert(types.begin(), types.end());
    }
    
    result.type.assign(typeSet.begin(), typeSet.end());
    
    // 收集所有时间戳
    std::set<std::int64_t> allTimestamps;
    
    // CPU时间戳
    for (const auto& point : metrics.cpu) {
        allTimestamps.insert(point.timestamp);
    }
    
    // Memory时间戳
    for (const auto& point : metrics.memory) {
        allTimestamps.insert(point.timestamp);
    }
    
    // Network时间戳
    for (const auto& [interface, points] : metrics.network) {
        for (const auto& point : points) {
            allTimestamps.insert(point.timestamp);
        }
    }
    
    // Disk时间戳
    for (const auto& [device, points] : metrics.disk) {
        for (const auto& point : points) {
            allTimestamps.insert(point.timestamp);
        }
    }
    
    // GPU时间戳
    for (const auto& [gpu_key, points] : metrics.gpu) {
        for (const auto& point : points) {
            allTimestamps.insert(point.timestamp);
        }
    }
    
    // 为每个时间戳创建数据点
    for (std::int64_t timestamp : allTimestamps) {
        ExportDataPoint dataPoint;
        dataPoint.timestamp = formatTimestamp(timestamp);
        
        // CPU数据
        for (const auto& point : metrics.cpu) {
            if (point.timestamp == timestamp) {
                dataPoint.cpu_usage_percent = point.usage_percent;
                break;
            }
        }
        
        // Memory数据
        for (const auto& point : metrics.memory) {
            if (point.timestamp == timestamp) {
                dataPoint.memory_usage_percent = point.usage_percent;
                break;
            }
        }
        
        // Network数据
        for (const auto& [interface, points] : metrics.network) {
            for (const auto& point : points) {
                if (point.timestamp == timestamp) {
                    dataPoint.network_rx_rate[interface] = static_cast<double>(point.rx_rate);
                    dataPoint.network_tx_rate[interface] = static_cast<double>(point.tx_rate);
                    break;
                }
            }
        }
        
        // Disk数据
        for (const auto& [device, points] : metrics.disk) {
            for (const auto& point : points) {
                if (point.timestamp == timestamp) {
                    dataPoint.disk_usage_percent[point.mount_point] = point.usage_percent;
                    break;
                }
            }
        }
        
        // GPU数据
        for (const auto& [gpu_key, points] : metrics.gpu) {
            for (const auto& point : points) {
                if (point.timestamp == timestamp) {
                    std::string gpu_index = std::to_string(point.index);
                    dataPoint.gpu_compute_usage[gpu_index] = point.compute_usage;
                    dataPoint.gpu_mem_usage[gpu_index] = point.mem_usage;
                    break;
                }
            }
        }
        
        result.data.push_back(dataPoint);
    }
    
    return result;
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
    
    // 辅助函数：验证百分比值（0-100）
    bool isValidPercent(double value) {
        return value >= 0.0 && value <= 100.0;
    }
    
    // 辅助函数：验证 Resource 数据
    std::string validateResourceData(const nlohmann::json& resourceJson) {
        // 检查必需字段 host_ip
        if (!resourceJson.contains("host_ip") || resourceJson["host_ip"].is_null()) {
            return "host_ip is required";
        }
        
        std::string hostIp = resourceJson["host_ip"].get<std::string>();
        if (hostIp.empty()) {
            return "host_ip cannot be empty";
        }
        
        // 验证 IP 地址格式
        if (!isValidIpAddress(hostIp)) {
            return "invalid host_ip format: " + hostIp;
        }
        
        // 验证 resource 字段（如果存在）
        if (resourceJson.contains("resource") && !resourceJson["resource"].is_null()) {
            const auto& res = resourceJson["resource"];
            
            // 验证 CPU 资源（如果存在）
            if (res.contains("cpu") && !res["cpu"].is_null()) {
                const auto& cpu = res["cpu"];
                if (cpu.contains("usage_percent") && !cpu["usage_percent"].is_null()) {
                    double usage = cpu["usage_percent"].get<double>();
                    if (!isValidPercent(usage)) {
                        return "cpu.usage_percent must be between 0 and 100, got: " + std::to_string(usage);
                    }
                }
                if (cpu.contains("core_count") && !cpu["core_count"].is_null()) {
                    int count = cpu["core_count"].get<int>();
                    if (count < 0) {
                        return "cpu.core_count must be non-negative, got: " + std::to_string(count);
                    }
                }
                if (cpu.contains("core_allocated") && !cpu["core_allocated"].is_null()) {
                    int allocated = cpu["core_allocated"].get<int>();
                    if (allocated < 0) {
                        return "cpu.core_allocated must be non-negative, got: " + std::to_string(allocated);
                    }
                }
            }
            
            // 验证 Memory 资源（如果存在）
            if (res.contains("memory") && !res["memory"].is_null()) {
                const auto& memory = res["memory"];
                if (memory.contains("usage_percent") && !memory["usage_percent"].is_null()) {
                    double usage = memory["usage_percent"].get<double>();
                    if (!isValidPercent(usage)) {
                        return "memory.usage_percent must be between 0 and 100, got: " + std::to_string(usage);
                    }
                }
                if (memory.contains("used") && memory.contains("total") && 
                    !memory["used"].is_null() && !memory["total"].is_null()) {
                    std::uint64_t used = memory["used"].get<std::uint64_t>();
                    std::uint64_t total = memory["total"].get<std::uint64_t>();
                    if (used > total) {
                        return "memory.used cannot be greater than memory.total";
                    }
                }
            }
            
            // 验证 Disk 数组（如果存在）
            if (res.contains("disk") && res["disk"].is_array()) {
                for (size_t i = 0; i < res["disk"].size(); ++i) {
                    const auto& disk = res["disk"][i];
                    if (disk.contains("usage_percent") && !disk["usage_percent"].is_null()) {
                        double usage = disk["usage_percent"].get<double>();
                        if (!isValidPercent(usage)) {
                            return "disk[" + std::to_string(i) + "].usage_percent must be between 0 and 100, got: " + std::to_string(usage);
                        }
                    }
                    if (disk.contains("used") && disk.contains("total") && 
                        !disk["used"].is_null() && !disk["total"].is_null()) {
                        std::uint64_t used = disk["used"].get<std::uint64_t>();
                        std::uint64_t total = disk["total"].get<std::uint64_t>();
                        if (used > total) {
                            return "disk[" + std::to_string(i) + "].used cannot be greater than disk[" + std::to_string(i) + "].total";
                        }
                    }
                }
            }
            
            // 验证 GPU 数组（如果存在）
            if (res.contains("gpu") && res["gpu"].is_array()) {
                for (size_t i = 0; i < res["gpu"].size(); ++i) {
                    const auto& gpu = res["gpu"][i];
                    if (gpu.contains("index") && !gpu["index"].is_null()) {
                        int index = gpu["index"].get<int>();
                        if (index < 0) {
                            return "gpu[" + std::to_string(i) + "].index must be non-negative, got: " + std::to_string(index);
                        }
                    }
                    if (gpu.contains("compute_usage") && !gpu["compute_usage"].is_null()) {
                        double usage = gpu["compute_usage"].get<double>();
                        if (!isValidPercent(usage)) {
                            return "gpu[" + std::to_string(i) + "].compute_usage must be between 0 and 100, got: " + std::to_string(usage);
                        }
                    }
                    if (gpu.contains("mem_usage") && !gpu["mem_usage"].is_null()) {
                        double usage = gpu["mem_usage"].get<double>();
                        if (!isValidPercent(usage)) {
                            return "gpu[" + std::to_string(i) + "].mem_usage must be between 0 and 100, got: " + std::to_string(usage);
                        }
                    }
                    if (gpu.contains("mem_used") && gpu.contains("mem_total") && 
                        !gpu["mem_used"].is_null() && !gpu["mem_total"].is_null()) {
                        std::uint64_t used = gpu["mem_used"].get<std::uint64_t>();
                        std::uint64_t total = gpu["mem_total"].get<std::uint64_t>();
                        if (used > total) {
                            return "gpu[" + std::to_string(i) + "].mem_used cannot be greater than gpu[" + std::to_string(i) + "].mem_total";
                        }
                    }
                }
            }
            
            // 验证 gpu_allocated 和 gpu_num（如果存在）
            if (res.contains("gpu_allocated") && !res["gpu_allocated"].is_null()) {
                int allocated = res["gpu_allocated"].get<int>();
                if (allocated < 0) {
                    return "gpu_allocated must be non-negative, got: " + std::to_string(allocated);
                }
            }
            if (res.contains("gpu_num") && !res["gpu_num"].is_null()) {
                int num = res["gpu_num"].get<int>();
                if (num < 0) {
                    return "gpu_num must be non-negative, got: " + std::to_string(num);
                }
            }
        }
        
        // 验证 component 数组（如果存在）
        if (resourceJson.contains("component") && resourceJson["component"].is_array()) {
            for (size_t i = 0; i < resourceJson["component"].size(); ++i) {
                const auto& comp = resourceJson["component"][i];
                if (comp.contains("resource") && !comp["resource"].is_null()) {
                    const auto& compRes = comp["resource"];
                    if (compRes.contains("memory") && !compRes["memory"].is_null()) {
                        const auto& mem = compRes["memory"];
                        if (mem.contains("mem_usage") && !mem["mem_usage"].is_null()) {
                            double usage = mem["mem_usage"].get<double>();
                            if (usage < 0.0 || usage > 1.0) {
                                return "component[" + std::to_string(i) + "].resource.memory.mem_usage must be between 0 and 1, got: " + std::to_string(usage);
                            }
                        }
                        if (mem.contains("mem_used") && mem.contains("mem_limit") && 
                            !mem["mem_used"].is_null() && !mem["mem_limit"].is_null()) {
                            std::uint64_t used = mem["mem_used"].get<std::uint64_t>();
                            std::uint64_t limit = mem["mem_limit"].get<std::uint64_t>();
                            if (used > limit) {
                                return "component[" + std::to_string(i) + "].resource.memory.mem_used cannot be greater than mem_limit";
                            }
                        }
                    }
                }
            }
        }
        
        return ""; // 验证通过
    }
}

void MonitorManager::setupRoutes() {
    if (!service_) {
        spdlog::error("HttpService not available for monitor routes");
        return;
    }

    service_->POST("/resource", [this](const HttpContextPtr& ctx) {
        try {
            auto body = ctx->body();
            
            // 检查请求体是否为空
            if (body.empty()) {
                spdlog::warn("Received empty resource request body");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "empty request body"},
                    {"data", json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 解析 JSON
            json j;
            try {
                j = json::parse(body);
            } catch (const json::exception& e) {
                spdlog::warn("Invalid JSON format in resource request: {}", e.what());
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "invalid JSON format: " + std::string(e.what())},
                    {"data", json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 检查是否包含 data 字段
            if (!j.contains("data") || j["data"].is_null()) {
                spdlog::warn("Missing 'data' field in resource request");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "missing 'data' field"},
                    {"data", json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 验证 data 字段中的数据
            std::string validationError = validateResourceData(j["data"]);
            if (!validationError.empty()) {
                spdlog::warn("Resource data validation failed: {}", validationError);
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "validation failed: " + validationError},
                    {"data", json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 尝试转换为 Resource 对象
            Resource res;
            try {
                res = j["data"].get<Resource>();
            } catch (const json::exception& e) {
                spdlog::error("Failed to parse Resource from JSON: {}", e.what());
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                ctx->setContentType("application/json");
                json resp = {
                    {"api_version", 1},
                    {"status", "error"},
                    {"message", "failed to parse resource data: " + std::string(e.what())},
                    {"data", json::object()}
                };
                return ctx->send(resp.dump(2));
            }
            
            // 写入内存缓存（使用当前秒时间）
            const auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::system_clock::now()
            ).time_since_epoch().count();
            if (monitor_cache_) {
                monitor_cache_->put(res, now_s);
            }
            
            // 保存到数据库
            if (repository_) {
                try {
                    repository_->save(res);
                } catch (const std::exception& e) {
                    spdlog::error("save resource failed: {}", e.what());
                    ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                    ctx->setContentType("application/json");
                    json resp = {
                        {"api_version", 1},
                        {"status", "error"},
                        {"message", "failed to save resource: " + std::string(e.what())},
                        {"data", json::object()}
                    };
                    return ctx->send(resp.dump(2));
                }
            }
            
            // 返回成功响应
            ctx->setContentType("application/json");
            json resp = {
                {"api_version", 1},
                {"status", "success"},
                {"message", "resource received"},
                {"data", json::object()}
            };
            return ctx->send(resp.dump(2));
            
        } catch (const std::exception& e) {
            spdlog::error("Unexpected error in resource handler: {}", e.what());
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            ctx->setContentType("application/json");
            json resp = {
                {"api_version", 1},
                {"status", "error"},
                {"message", "internal server error: " + std::string(e.what())},
                {"data", json::object()}
            };
            return ctx->send(resp.dump(2));
        }
    });
 
    // GET /resource 路由迁移至 WebController
}

} // namespace monitor
} // namespace yw


