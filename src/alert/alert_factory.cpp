#include "yw/alert.h"
#include "AlertManager.h"
#include <mutex>

namespace yw {
namespace alert {

std::shared_ptr<IAlertModule> AlertFactory::getAlertModule() {
    // 在 AlertManager 内部创建数据库连接
    return std::make_shared<AlertManager>();
}

} // namespace alert
} // namespace yw
