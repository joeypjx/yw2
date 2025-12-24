#include "monitor_manager.h"
#include "monitor/monitor_model.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <set>
#include <unordered_map>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include "utils/multicast_scanner.h"
#include "utils/json_config.h"
#include "utils/duration_utils.h"
#include <regex>

namespace yw {
namespace monitor {

using json = nlohmann::json;

MonitorManager::MonitorManager(std::shared_ptr<hv::HttpService> service)
    : service_(std::move(service)) {
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
    auto t_func_start = std::chrono::high_resolution_clock::now();
    spdlog::info("[exportNodeData] 函数开始，host_ip: {}, start_time: {}, end_time: {}", host_ip, start_time, end_time);
    
    ExportData result;
    result.ip = host_ip;
    
    // 转换时间戳为字符串格式（带缓存）
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
    
    auto t_query_start = std::chrono::high_resolution_clock::now();
    // 查询原始数据
    MetricsSeries metrics = repository_->queryMetricsSeries(host_ip, start_time, end_time, types);
    auto t_query_end = std::chrono::high_resolution_clock::now();
    auto query_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_query_end - t_query_start).count();
    spdlog::info("[exportNodeData] 数据库查询完成，耗时: {} ms，CPU点数: {}, Memory点数: {}, Disk设备数: {}, Network接口数: {}, GPU数: {}", 
                 query_duration, metrics.cpu.size(), metrics.memory.size(), metrics.disk.size(), metrics.network.size(), metrics.gpu.size());
    
    auto t_index_start = std::chrono::high_resolution_clock::now();
    
    // ========== 使用哈希表建立时间戳索引 ==========
    
    // CPU 索引：timestamp -> CpuPoint
    std::unordered_map<std::int64_t, const CpuPoint*> cpuIndex;
    for (const auto& point : metrics.cpu) {
        cpuIndex[point.timestamp] = &point;
    }
    
    // Memory 索引：timestamp -> MemoryPoint
    std::unordered_map<std::int64_t, const MemoryPoint*> memoryIndex;
    for (const auto& point : metrics.memory) {
        memoryIndex[point.timestamp] = &point;
    }
    
    // Network 索引：timestamp -> map<interface, NetworkPoint>
    std::unordered_map<std::int64_t, std::unordered_map<std::string, const NetworkPoint*>> networkIndex;
    for (const auto& [interface, points] : metrics.network) {
        for (const auto& point : points) {
            networkIndex[point.timestamp][interface] = &point;
        }
    }
    
    // Disk 索引：timestamp -> map<mount_point, DiskPoint>
    std::unordered_map<std::int64_t, std::unordered_map<std::string, const DiskPoint*>> diskIndex;
    for (const auto& [device, points] : metrics.disk) {
        for (const auto& point : points) {
            diskIndex[point.timestamp][point.mount_point] = &point;
        }
    }
    
    // GPU 索引：timestamp -> map<gpu_index, GpuPoint>
    std::unordered_map<std::int64_t, std::unordered_map<int, const GpuPoint*>> gpuIndex;
    for (const auto& [gpu_key, points] : metrics.gpu) {
        for (const auto& point : points) {
            gpuIndex[point.timestamp][point.index] = &point;
        }
    }
    
    // 收集所有唯一时间戳（由于各指标的时间戳一致，只需使用 CPU 的时间戳）
    std::set<std::int64_t> allTimestamps;
    for (const auto& [ts, _] : cpuIndex) {
        allTimestamps.insert(ts);
    }
    
    auto t_index_end = std::chrono::high_resolution_clock::now();
    auto index_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_index_end - t_index_start).count();
    spdlog::info("[exportNodeData] 索引构建完成，耗时: {} ms，唯一时间戳数量: {}", index_duration, allTimestamps.size());
    
    // ========== 构建类型列表 ==========
    auto t_type_start = std::chrono::high_resolution_clock::now();
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
    auto t_type_end = std::chrono::high_resolution_clock::now();
    auto type_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_type_end - t_type_start).count();
    spdlog::info("[exportNodeData] 类型列表构建完成，耗时: {} ms", type_duration);
    
    // ========== 缓存时间戳格式化结果 ==========
    auto t_format_start = std::chrono::high_resolution_clock::now();
    std::unordered_map<std::int64_t, std::string> timestampCache;
    for (std::int64_t ts : allTimestamps) {
        timestampCache[ts] = formatTimestamp(ts);
    }
    auto t_format_end = std::chrono::high_resolution_clock::now();
    auto format_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_format_end - t_format_start).count();
    spdlog::info("[exportNodeData] 时间戳格式化缓存完成，耗时: {} ms", format_duration);
    
    // ========== 使用索引快速构建数据点 ==========
    auto t_datapoint_start = std::chrono::high_resolution_clock::now();
    result.data.reserve(allTimestamps.size());  // 预分配空间
    
    for (std::int64_t timestamp : allTimestamps) {
        ExportDataPoint dataPoint;
        dataPoint.timestamp = timestampCache[timestamp];
        
        // CPU数据 - O(1) 查找
        if (auto it = cpuIndex.find(timestamp); it != cpuIndex.end()) {
            dataPoint.cpu_usage_percent = it->second->usage_percent;
        }
        
        // Memory数据 - O(1) 查找
        if (auto it = memoryIndex.find(timestamp); it != memoryIndex.end()) {
            dataPoint.memory_usage_percent = it->second->usage_percent;
        }
        
        // Network数据 - O(1) 查找
        if (auto it = networkIndex.find(timestamp); it != networkIndex.end()) {
            for (const auto& [interface, point] : it->second) {
                dataPoint.network_rx_rate[interface] = static_cast<double>(point->rx_rate);
                dataPoint.network_tx_rate[interface] = static_cast<double>(point->tx_rate);
            }
        }
        
        // Disk数据 - O(1) 查找
        if (auto it = diskIndex.find(timestamp); it != diskIndex.end()) {
            for (const auto& [mount_point, point] : it->second) {
                dataPoint.disk_usage_percent[mount_point] = point->usage_percent;
            }
        }
        
        // GPU数据 - O(1) 查找
        if (auto it = gpuIndex.find(timestamp); it != gpuIndex.end()) {
            for (const auto& [gpu_idx, point] : it->second) {
                std::string gpu_index_str = std::to_string(gpu_idx);
                dataPoint.gpu_compute_usage[gpu_index_str] = point->compute_usage;
                dataPoint.gpu_mem_usage[gpu_index_str] = point->mem_usage;
            }
        }
        
        result.data.push_back(std::move(dataPoint));
    }
    
    auto t_datapoint_end = std::chrono::high_resolution_clock::now();
    auto datapoint_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_datapoint_end - t_datapoint_start).count();
    spdlog::info("[exportNodeData] 数据点构建完成，耗时: {} ms，数据点数量: {}", datapoint_duration, result.data.size());
    
    auto t_func_end = std::chrono::high_resolution_clock::now();
    auto func_duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_func_end - t_func_start).count();
    spdlog::info("[exportNodeData] 函数完成，总耗时: {} ms", func_duration);
    
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
                
        return ""; // 验证通过
    }
    
    // 辅助函数：发送错误响应
    int sendErrorResponse(const HttpContextPtr& ctx, http_status status, const std::string& message) {
        ctx->setStatus(status);
        ctx->setContentType("application/json");
        json resp = {
            {"api_version", 1},
            {"status", "error"},
            {"message", message},
            {"data", json::object()}
        };
        return ctx->send(resp.dump(2));
    }
    
    // 辅助函数：发送成功响应
    int sendSuccessResponse(const HttpContextPtr& ctx, const std::string& message = "resource received") {
        ctx->setContentType("application/json");
        json resp = {
            {"api_version", 1},
            {"status", "success"},
            {"message", message},
            {"data", json::object()}
        };
        return ctx->send(resp.dump(2));
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
                return sendErrorResponse(ctx, HTTP_STATUS_BAD_REQUEST, "empty request body");
            }
            
            // 解析 JSON
            json j;
            try {
                j = json::parse(body);
            } catch (const json::exception& e) {
                spdlog::warn("Invalid JSON format in resource request: {}", e.what());
                return sendErrorResponse(ctx, HTTP_STATUS_BAD_REQUEST, "invalid JSON format: " + std::string(e.what()));
            }
            
            // 检查是否包含 data 字段
            if (!j.contains("data") || j["data"].is_null()) {
                spdlog::warn("Missing 'data' field in resource request");
                return sendErrorResponse(ctx, HTTP_STATUS_BAD_REQUEST, "missing 'data' field");
            }
            
            // 验证 data 字段中的数据
            std::string validationError = validateResourceData(j["data"]);
            if (!validationError.empty()) {
                spdlog::warn("Resource data validation failed: {}", validationError);
                return sendErrorResponse(ctx, HTTP_STATUS_BAD_REQUEST, "validation failed: " + validationError);
            }
            
            // 尝试转换为 Resource 对象
            Resource res;
            try {
                res = j["data"].get<Resource>();
            } catch (const json::exception& e) {
                spdlog::error("Failed to parse Resource from JSON: {}", e.what());
                return sendErrorResponse(ctx, HTTP_STATUS_BAD_REQUEST, "failed to parse resource data: " + std::string(e.what()));
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
                    return sendErrorResponse(ctx, HTTP_STATUS_INTERNAL_SERVER_ERROR, "failed to save resource: " + std::string(e.what()));
                }
            }
            
            // 返回成功响应
            return sendSuccessResponse(ctx);
            
        } catch (const std::exception& e) {
            spdlog::error("Unexpected error in resource handler: {}", e.what());
            return sendErrorResponse(ctx, HTTP_STATUS_INTERNAL_SERVER_ERROR, "internal server error: " + std::string(e.what()));
        }
    });
}

} // namespace monitor
} // namespace yw


