#pragma once

#include <memory>
#include <hv/HttpService.h>
#include <hv/HttpServer.h>
#include "web/web.h"
#include "node/node.h"
#include "monitor/monitor.h"
#include "bmc/bmc.h"
#include "controller/controller.h"
#include "services/alert_pusher.h"

// Forward declaration for AlertV2
namespace yw {
namespace alert {
    class IAlertModule;
}
}

namespace yw {
namespace web {

class WebController : public IWebModule {
public:
    WebController(std::shared_ptr<hv::HttpServer> server,
                  std::shared_ptr<hv::HttpService> service,
                  std::shared_ptr<node::INodeModule> node_module,
                  std::shared_ptr<monitor::IMonitorModule> monitor_module,
                  std::shared_ptr<bmc::IBMCModule> bmc_module,
                  std::shared_ptr<controller::IControllerModule> controller_module,
                  std::shared_ptr<alert::IAlertModule> alert_module = nullptr);

    ~WebController() override;

    void setupRoutes() override;

    

private:
    std::shared_ptr<hv::HttpServer>         server_;
    std::shared_ptr<hv::HttpService>        service_;
    std::shared_ptr<node::INodeModule>      node_module_;
    std::shared_ptr<monitor::IMonitorModule> monitor_module_;
    std::shared_ptr<bmc::IBMCModule>        bmc_module_;
    std::shared_ptr<controller::IControllerModule> controller_module_;
    std::shared_ptr<alert::IAlertModule> alert_module_;

    std::unique_ptr<AlertPusher>            pusher_;
};

} // namespace web
} // namespace yw


