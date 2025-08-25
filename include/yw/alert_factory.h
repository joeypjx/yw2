#pragma once

#include <memory>
#include "alert.h"

namespace yw {
namespace alert {

class AlertFactory {
public:
    static std::shared_ptr<IAlertModule> getAlertModule();
};

} // namespace alert
} // namespace yw
