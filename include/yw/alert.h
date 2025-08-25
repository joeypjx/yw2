#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

#include "alert_model.h"

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
    virtual std::vector<AlertState> listActiveAlerts(const LabelSet& matcher) const = 0;
    virtual std::vector<AlertEvent> queryEvents(const std::string& duration) const = 0; // e.g. "1h"
    virtual bool ackAlert(const std::string& fingerprint, const std::string& user, const std::string& comment) = 0;
};

} // namespace alert
} // namespace yw

// 包含 AlertFactory 定义
#include "alert_factory.h"


