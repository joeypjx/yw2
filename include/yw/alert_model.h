#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "alert_types.h"

namespace yw {
namespace alert {

struct Rule {
    std::string                 id;
    std::string                 name;
    std::string                 description;
    std::string                 expression;
    std::string                 window;
    std::string                 eval_every;
    Severity                    severity = Severity::Warn;
    std::string                 tag;
    LabelSet                    selector;
    std::int32_t                for_times = 1;
    bool                        enabled = true;
    std::string                 created_at;
    std::string                 updated_at;
};

struct AlertState {
    std::string                 fingerprint;
    std::string                 rule_id;
    AlertStatus                 status = AlertStatus::Inactive;
    Severity                    severity = Severity::Warn;
    LabelSet                    labels;
    std::int64_t                first_firing_ms = 0;
    std::int64_t                last_eval_ms = 0;
    std::int64_t                last_change_ms = 0;
    std::int64_t                notify_cooldown_ms = 0;
    std::int64_t                occurrences = 0;
    bool                        acked = false;
    std::string                 acked_by;
    std::int64_t                acked_at_ms = 0;
};

struct AlertEvent {
    std::int64_t                timestamp_ms = 0;
    std::int64_t                resolved_timestamp_ms = 0;
    std::string                 fingerprint;
    std::string                 rule_id;
    std::string                 action;
    AlertStatus                 status = AlertStatus::Inactive;
    Severity                    severity = Severity::Warn;
    LabelSet                    labels;
    std::string                 title;
    std::string                 description;
    double                      value = 0.0;
    std::string                 unit;
    nlohmann::json              context;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rule,
    id, name, description, expression, window, eval_every, severity, tag, selector, for_times, enabled, created_at, updated_at)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertState,
    fingerprint, rule_id, status, severity, labels,
    first_firing_ms, last_eval_ms, last_change_ms, notify_cooldown_ms,
    occurrences, acked, acked_by, acked_at_ms)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEvent,
    timestamp_ms, resolved_timestamp_ms, fingerprint, rule_id, action, status, severity,
    labels, title, description, value, unit, context)

} // namespace alert
} // namespace yw

// 对外 DTO 仍通过专门头导出
#include "../../src/web/dto/alert_dto.h"