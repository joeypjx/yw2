// ============================================================================
// 文件功能描述：
// Web控制器（WebController）的实现文件，负责HTTP路由的注册和Web服务的统一管理。
// 主要功能包括：
// 1. 路由注册：统一注册所有业务模块的HTTP路由（节点、监控、BMC、告警等）
// 2. CORS支持：配置HTTP服务允许跨域请求，方便前端调用
// 3. 告警推送集成：创建AlertPusher实例，实现告警事件的WebSocket推送功能
// 4. 模块依赖注入：接收并管理所有业务模块的引用，提供给路由处理器使用
// 5. 回调设置：将Web层的推送能力注入到告警模块，实现告警事件的实时推送
// 6. 生命周期管理：在析构时自动停止告警推送器，确保资源正确释放
// ============================================================================

#include "web_controller.h"
#include <hv/HttpService.h>
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "bmc/bmc_model.h"
#include "controller/controller.h"
#include "mapper/node_mapper.h"
#include "routes/node_routes.h"
#include "routes/metrics_routes.h"
#include "routes/bmc_routes.h"
#include "routes/alert_routes.h"
#include "alert/alert.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <spdlog/spdlog.h>

namespace yw {
namespace web {

using json = nlohmann::json;

// Web控制器构造函数，初始化HTTP服务和路由
// server: HTTP服务器实例
// service: HTTP服务实例
// node_module: 节点模块
// monitor_module: 监控模块
// bmc_module: BMC模块
// controller_module: 控制器模块
// alert_module: 告警模块
WebController::WebController(std::shared_ptr<hv::HttpServer> server,
                             std::shared_ptr<hv::HttpService> service,
                             std::shared_ptr<node::INodeModule> node_module,
                             std::shared_ptr<monitor::IMonitorModule> monitor_module,
                             std::shared_ptr<bmc::IBMCModule> bmc_module,
                             std::shared_ptr<controller::IControllerModule> controller_module,
                             std::shared_ptr<alert::IAlertModule> alert_module)
    : server_(std::move(server)),
      service_(std::move(service)),
      node_module_(std::move(node_module)),
      monitor_module_(std::move(monitor_module)),
      bmc_module_(std::move(bmc_module)),
      controller_module_(std::move(controller_module)),
      alert_module_(std::move(alert_module)) {

    pusher_ = std::make_unique<AlertPusher>(server_.get());

    if (service_) {
        service_->AllowCORS();
        setupRoutes();
    }

    // 将 Web 层推送能力注入到 alert 模块
    if (alert_module_) {
        alert_module_->setPushCallback([this](const nlohmann::json& alertJson){
            if (pusher_) {
                pusher_->pushJson(alertJson);
            }
        });
    }
}

// Web控制器析构函数，停止告警推送器
WebController::~WebController() {
    if (pusher_) {
        pusher_->stop();
    }
}

// 设置HTTP路由，注册所有API端点
void WebController::setupRoutes() {
    // 仅负责装配分域路由
    routes::registerNodeRoutes(service_.get(), node_module_.get(), monitor_module_.get(), bmc_module_.get());
    routes::registerMetricsRoutes(service_.get(), node_module_.get(), monitor_module_.get(), bmc_module_.get());
    routes::registerBMCRoutes(service_.get(), bmc_module_.get(), controller_module_.get());
    
    // 注册告警路由（如果告警模块可用）
    if (alert_module_) {
        routes::registerAlertRoutes(service_.get(), alert_module_);
        spdlog::info("Alert routes registered successfully");
    } else {
        spdlog::warn("Alert module not available, skipping alert routes");
    }
}



} // namespace web
} // namespace yw


