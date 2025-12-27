// ============================================================================
// 文件功能描述：
// 控制器工厂（ControllerFactory）的实现文件，负责创建和初始化控制器模块实例。
// 主要功能包括：
// 1. 模块创建：创建ResourceController实例，提供板卡控制功能
// 2. 工厂模式：提供统一的模块创建接口，隐藏具体的实现细节
// ============================================================================

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

