#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace yw {
namespace alert {

using LabelSet = std::unordered_map<std::string, std::string>;

enum class Severity { Info, Warn, Critical };
enum class AlertStatus { Inactive, Pending, Firing, Resolved };

NLOHMANN_JSON_SERIALIZE_ENUM(Severity, {
    {Severity::Info,      "提示"},
    {Severity::Warn,      "一般"},
    {Severity::Critical,  "严重"}
})

NLOHMANN_JSON_SERIALIZE_ENUM(AlertStatus, {
    {AlertStatus::Inactive, "未触发"},
    {AlertStatus::Pending,  "待触发"},
    {AlertStatus::Firing,   "触发中"},
    {AlertStatus::Resolved, "已解决"}
})

struct Rule {
    std::string                 id;             // 规则唯一 ID
    std::string                 name;           // 规则名称
    std::string                 description;    // 规则描述
    std::string                 expression;     // 规则表达式（SQL/DSL）
    std::string                 window;         // 统计窗口，如 "5m"
    std::string                 eval_every;     // 评估周期，如 "30s"
    Severity                    severity = Severity::Warn;
    std::string                 tag;            // 规则标签
    LabelSet                    selector;       // 标签选择器
    std::int32_t                for_times = 1;  // 连续命中次数才触发
    bool                        enabled = true; // 是否启用
};

struct AlertState {
    std::string                 fingerprint;        // 指纹 = rule_id + 标签
    std::string                 rule_id;
    AlertStatus                 status = AlertStatus::Inactive;
    Severity                    severity = Severity::Warn;
    LabelSet                    labels;             // 实例标签
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
    std::int64_t                timestamp_ms = 0;   // 事件时间
    std::int64_t                resolved_timestamp_ms = 0; // 解决时间（毫秒，0 表示未解决）
    std::string                 fingerprint;
    std::string                 rule_id;
    std::string                 action;             // firing/resolved/ack/notified/escalated
    AlertStatus                 status = AlertStatus::Inactive;
    Severity                    severity = Severity::Warn;
    LabelSet                    labels;
    std::string                 title;
    std::string                 description;
    double                      value = 0.0;        // 触发值（可选）
    std::string                 unit;               // 单位（可选）
    nlohmann::json              context;            // 上下文（查询片段/可视化链接等）
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rule,
    id, name, description, expression, window, eval_every, severity, tag, selector, for_times, enabled)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertState,
    fingerprint, rule_id, status, severity, labels,
    first_firing_ms, last_eval_ms, last_change_ms, notify_cooldown_ms,
    occurrences, acked, acked_by, acked_at_ms)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEvent,
    timestamp_ms, resolved_timestamp_ms, fingerprint, rule_id, action, status, severity,
    labels, title, description, value, unit, context)

} // namespace alert
} // namespace yw


