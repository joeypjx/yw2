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


