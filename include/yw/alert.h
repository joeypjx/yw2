#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>
#include <functional>

#include "alert_types.h"
#include "alert_model.h"

namespace hv {
class HttpServer;
}

namespace yw {
namespace alert {

class IAlertModule {
public:
    virtual ~IAlertModule() = default;

    // 规则管理
    virtual std::vector<Rule> listRules() const = 0;
    virtual std::optional<Rule> getRule(const std::string& id) const = 0;
    virtual bool upsertRule(const Rule& rule) = 0;
    virtual bool deleteRule(const std::string& id) = 0;

    // 告警查询与操作
    virtual std::vector<AlertEvent> queryEvents(const std::string& duration) const = 0; // e.g. "1h"
    // 统计：返回指定状态的事件总量（全量，不限时间）
    virtual std::size_t countEventsByStatus(AlertStatus status) const = 0;

    // 手动处理告警事件
    virtual bool appendAlertEvent(const AlertEvent& event) = 0;


    // 设置外部推送回调（由 Web 层提供）
    virtual void setPushCallback(std::function<void(const AlertEvent&)> cb) = 0;
};

class AlertFactory {
    public:
        static std::shared_ptr<IAlertModule> getAlertModule();
};

} // namespace alert
} // namespace yw


