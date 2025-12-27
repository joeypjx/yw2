// ============================================================================
// 文件功能描述：
// 监控工厂（MonitorFactory）的实现文件，负责创建和初始化监控模块实例。
// 主要功能包括：
// 1. 模块创建：创建MonitorManager实例，传入HTTP服务用于注册API路由
// 2. 工厂模式：提供统一的模块创建接口，隐藏具体的实现细节
// 3. 依赖注入：接收HTTP服务实例，实现模块间的松耦合
// ============================================================================

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

