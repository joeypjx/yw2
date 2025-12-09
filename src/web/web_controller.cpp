#include "web_controller.h"
#include <hv/HttpService.h>
#include "yw/node.h"
#include "yw/monitor.h"
#include "yw/bmc.h"
#include "yw/bmc_model.h"
#include "yw/controller.h"
#include "dto/node_dto.h"
#include "mapper/NodeMapper.h"
#include "routes/NodeRoutes.h"
#include "routes/MetricsRoutes.h"
#include "routes/BMCRoutes.h"
#include "routes/AlertRoutes.h"
#include "../../alert/application/AlertEngine.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <spdlog/spdlog.h>

namespace yw {
namespace web {

using json = nlohmann::json;

WebController::WebController(std::shared_ptr<hv::HttpServer> server,
                             std::shared_ptr<hv::HttpService> service,
                             std::shared_ptr<node::INodeModule> node_module,
                             std::shared_ptr<monitor::IMonitorModule> monitor_module,
                             std::shared_ptr<bmc::IBMCModule> bmc_module,
                             std::shared_ptr<controller::IControllerModule> controller_module,
                             std::shared_ptr<alert::AlertEngine> alert_engine)
    : server_(std::move(server)),
      service_(std::move(service)),
      node_module_(std::move(node_module)),
      monitor_module_(std::move(monitor_module)),
      bmc_module_(std::move(bmc_module)),
      controller_module_(std::move(controller_module)),
      alert_engine_(std::move(alert_engine)) {

    pusher_ = std::make_unique<AlertPusher>(server_.get());

    if (service_) {
        service_->AllowCORS();
        setupRoutes();
    }

    // 将 Web 层推送能力注入到 alertv2 引擎
    if (alert_engine_) {
        alert_engine_->setPushCallback([this](const alert::Alert& alert){
            if (pusher_) {
                pusher_->pushV2(alert);
            }
        });
    }
}

WebController::~WebController() {
    if (pusher_) {
        pusher_->stop();
    }
}

void WebController::setupRoutes() {
    // 仅负责装配分域路由
    routes::registerNodeRoutes(service_.get(), node_module_.get(), monitor_module_.get(), bmc_module_.get());
    routes::registerMetricsRoutes(service_.get(), node_module_.get(), monitor_module_.get(), bmc_module_.get());
    routes::registerBMCRoutes(service_.get(), bmc_module_.get(), controller_module_.get());
    
    // 注册AlertV2路由（如果AlertV2引擎可用）
    if (alert_engine_) {
        routes::registerAlertRoutes(service_.get(), alert_engine_);
        spdlog::info("AlertV2 routes registered successfully");
    } else {
        spdlog::warn("AlertV2 engine not available, skipping AlertV2 routes");
    }
}



} // namespace web
} // namespace yw


