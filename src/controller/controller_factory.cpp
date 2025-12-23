#include "controller/controller.h"
#include "resource_controller.h"

namespace yw {
namespace controller {

std::shared_ptr<IControllerModule> ControllerFactory::getControllerModule() {
    return std::make_shared<ResourceController>();
}

} // namespace controller
} // namespace yw

