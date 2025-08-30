#include "monitor_manager.h"
#include "yw/monitor_model.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include "yw/MulticastScanner.h"
#include "yw/node.h"
#include "yw/JsonConfig.h"

namespace yw {
namespace monitor {

using json = nlohmann::json;

MonitorManager::MonitorManager(std::shared_ptr<hv::HttpService> service,
                               std::shared_ptr<node::INodeModule> node_module)
    : service_(std::move(service)), node_module_(std::move(node_module)) {
    service_->AllowCORS();
    
    // 启动资源扫描器（与 NodeManager 配置保持一致；从配置读取）
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        yw::utils::JsonConfig::Get<std::string>("scanner.manager_ip",
            yw::utils::JsonConfig::Get<std::string>("host", "0.0.0.0")),
        yw::utils::JsonConfig::Get<int>("scanner.manager_port",
            yw::utils::JsonConfig::Get<int>("port", 18888)),
        yw::utils::JsonConfig::Get<std::string>("scanner.url_resource", "/resource"),
        yw::utils::JsonConfig::Get<std::string>("scanner.multicast_ip", "239.192.168.80"),
        yw::utils::JsonConfig::Get<int>("scanner.multicast_port", 3980),
        yw::utils::JsonConfig::Get<int>("scanner.interval_ms", 3000)
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

    auto normalizeDuration = [](std::string in) -> std::string {
        if (in.empty()) return std::string("1 minute");
        auto trim = [](std::string& s){
            size_t a = s.find_first_not_of(" \t\n\r");
            size_t b = s.find_last_not_of(" \t\n\r");
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
        };
        trim(in);
        if (in.empty()) return std::string("1 minute");
        char u = in.back();
        if (u=='s' || u=='S' || u=='m' || u=='M' || u=='h' || u=='H') {
            std::string num = in.substr(0, in.size()-1);
            trim(num);
            if (num.empty()) return std::string("1 minute");
            bool ok = true; for (char ch : num) { if (ch<'0'||ch>'9') { ok=false; break; } }
            if (!ok) return in;
            switch (u) {
                case 's': case 'S': return num + " seconds";
                case 'm': case 'M': return num + " minutes";
                case 'h': case 'H': return num + " hours";
            }
        }
        return in;
    };
    std::string intervalStr = normalizeDuration(duration);

    return repository_->queryMetricsSeries(host_ip, intervalStr, kinds);
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


