#pragma once

#include <memory>
#include <hv/HttpService.h>
#include <hv/HttpServer.h>
#include "yw/web.h"
#include "yw/node.h"
#include "yw/monitor.h"
#include "yw/bmc.h"
#include "dto/node_dto.h"
#include "AlertPusher.h"

// Forward declaration for AlertV2
namespace yw {
namespace alert {
    class AlertEngine;
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
                  std::shared_ptr<alert::AlertEngine> alert_engine = nullptr);

    ~WebController() override;

    void setupRoutes() override;

    

private:
    std::shared_ptr<hv::HttpServer>         server_;
    std::shared_ptr<hv::HttpService>        service_;
    std::shared_ptr<node::INodeModule>      node_module_;
    std::shared_ptr<monitor::IMonitorModule> monitor_module_;
    std::shared_ptr<bmc::IBMCModule>        bmc_module_;
    std::shared_ptr<alert::AlertEngine>  alert_engine_;

    std::unique_ptr<AlertPusher>            pusher_;
};

} // namespace web
} // namespace yw


