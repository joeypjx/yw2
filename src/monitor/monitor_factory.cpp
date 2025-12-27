#include "monitor/monitor.h"
#include "monitor_manager.h"
#include <mutex>
#include <hv/HttpService.h>

namespace yw {
namespace monitor {

// 创建监控模块实例
// service: HTTP服务实例，用于注册监控相关的API路由
// 返回: 监控模块共享指针
std::shared_ptr<IMonitorModule> MonitorFactory::getMonitorModule(std::shared_ptr<hv::HttpService> service) {
    return std::make_shared<MonitorManager>(service);
}

} // namespace monitor
} // namespace yw

