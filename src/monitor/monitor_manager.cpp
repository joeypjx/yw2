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

void MonitorManager::setupRoutes() {
    if (!service_) {
        spdlog::error("HttpService not available for monitor routes");
        return;
    }

    service_->POST("/resource", [this](const HttpContextPtr& ctx) {
        // 解析请求体并提取 data -> Resource
        const auto j = json::parse(ctx->body());
        if (j.contains("data")) {
            const Resource res = j["data"].get<Resource>();
            // 写入内存缓存（使用当前毫秒时间）
            const auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::system_clock::now()
            ).time_since_epoch().count();
            if (monitor_cache_) {
                monitor_cache_->put(res, now_s);
            }
            if (repository_) {
                try {
                    repository_->save(res);
                } catch (const std::exception& e) {
                    spdlog::error("save resource failed: {}", e.what());
                    return 500;
                }
            }
        }
        return 200;
    });
 
    // GET /resource 路由迁移至 WebController
}

} // namespace monitor
} // namespace yw


