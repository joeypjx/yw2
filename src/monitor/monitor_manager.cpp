#include "monitor_manager.h"
#include "yw/monitor_model.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include "yw/MulticastScanner.h"
#include "yw/node.h"

namespace yw {
namespace monitor {

using json = nlohmann::json;

MonitorManager::MonitorManager(std::shared_ptr<hv::HttpService> service,
                               std::shared_ptr<node::INodeModule> node_module)
    : service_(std::move(service)), node_module_(std::move(node_module)) {
    service_->AllowCORS();
    
    // 启动资源扫描器（示例 manager_ip 与端口与 NodeManager 保持一致，路由改为 /resource）
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        "192.168.10.254",
        8080,
        "/resource"
    );
    scanner_->start();

    // 初始化仓库（连接串可改为配置项）
    repository_ = std::make_unique<ResourceRepository>(
        "postgres://postgres:HZ715Net@localhost:5432/resource"
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

void MonitorManager::setupRoutes() {
    if (!service_) {
        std::cerr << "HttpService not available for monitor routes" << std::endl;
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
                    std::cerr << "save resource failed: " << e.what() << std::endl;
                    return 500;
                }
            }
        }
        return 200;
    });

    // 查询近一段时间的资源序列
    service_->GET("/resource", [this](const HttpContextPtr& ctx) {
        if (!repository_) return 500;

        // 读取查询参数：ip（必填）、duration（默认1 minute）、kinds（可选，逗号分隔）
        std::string ip;
        std::string duration = "1 minute";
        std::vector<std::string> kinds;

        auto params = ctx->params();
        if (params.find("ip") != params.end()) ip = params["ip"]; // host_ip
        if (params.find("duration") != params.end()) duration = params["duration"]; // e.g. 30 seconds / 1 minute
        if (params.find("kinds") != params.end()) {
            const std::string& ks = params["kinds"]; // e.g. cpu,memory,network
            std::string item;
            for (size_t i = 0, n = ks.size(); i <= n; ++i) {
                if (i == n || ks[i] == ',') {
                    if (!item.empty()) kinds.push_back(item);
                    item.clear();
                } else {
                    item.push_back(ks[i]);
                }
            }
        }

        if (ip.empty()) {
            return ctx->send("{\"error\":\"missing ip\"}");
        }

        // 允许简写：1h/5m/10s（仅单一单位）。转换为 PostgreSQL interval 字符串
        auto normalizeDuration = [](std::string in) -> std::string {
            if (in.empty()) return std::string("1 minute");
            // 去掉首尾空白
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
                // 简单校验数字
                bool ok = true; for (char ch : num) { if (ch<'0'||ch>'9') { ok=false; break; } }
                if (!ok) return in; // 交给数据库解析
                switch (u) {
                    case 's': case 'S': return num + " seconds";
                    case 'm': case 'M': return num + " minutes";
                    case 'h': case 'H': return num + " hours";
                }
            }
            // 非简写，直接交给数据库解析（如 "30 seconds"）
            return in;
        };
        std::string intervalStr = normalizeDuration(duration);

        try {
            MetricsSeries series = repository_->queryMetricsSeries(ip, intervalStr, kinds);

            // 组装 ResourceWindow，部分信息来自 node_module_
            ResourceWindow win;
            win.host_ip = ip;
            win.metrics = std::move(series);
            win.time_range = duration; // 保留原查询表达（如 1m/30s），仅作展示
            if (node_module_) {
                auto nodeOpt = node_module_->getNodeByIP(ip);
                if (nodeOpt) {
                    win.box_id = nodeOpt->box_id;
                    win.cpu_id = nodeOpt->cpu_id;
                    win.slot_id = nodeOpt->slot_id;
                }
            }

            nlohmann::json resp = win;
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            std::cerr << "query resource failed: " << e.what() << std::endl;
            return 500;
        }
    });
}

} // namespace monitor
} // namespace yw


