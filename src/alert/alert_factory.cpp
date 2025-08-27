#include "yw/alert.h"
#include "AlertManager.h"
#include <mutex>

namespace yw {
namespace alert {

std::shared_ptr<IAlertModule> AlertFactory::getAlertModule() {
    static std::shared_ptr<IAlertModule> instance;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        // 在 AlertManager 内部创建数据库连接
        instance = std::make_shared<AlertManager>();
    }
    return instance;
}

} // namespace alert
} // namespace yw
