#include "monitor_manager.h"
#include "monitor_model.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <hv/HttpServer.h>
#include <hv/HttpService.h>
#include "yw/MulticastScanner.h"

namespace yw {
namespace monitor {

using json = nlohmann::json;

MonitorManager::MonitorManager(std::shared_ptr<hv::HttpService> service)
    : service_(std::move(service)) {
    service_->AllowCORS();
    
    // 启动资源扫描器（示例 manager_ip 与端口与 NodeManager 保持一致，路由改为 /resource）
    scanner_ = std::make_unique<yw::utils::MulticastScanner>(
        "192.168.10.254",
        8080,
        "/resource"
    );
    scanner_->start();

    setupRoutes();
}

MonitorManager::~MonitorManager() = default;

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
            (void)res; // TODO: 交给后续监控处理逻辑
        }
        return 200;
    });
}

} // namespace monitor
} // namespace yw


