#include "controller/controller.h"
#include "resource_controller.h"

namespace yw {
namespace controller {

// 创建控制器模块实例
// 返回: 控制器模块共享指针
std::shared_ptr<IControllerModule> ControllerFactory::getControllerModule() {
    return std::make_shared<ResourceController>();
}

} // namespace controller
} // namespace yw

