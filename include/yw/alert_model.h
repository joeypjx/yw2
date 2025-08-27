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
    {AlertStatus::Inactive, "inactive"},
    {AlertStatus::Pending,  "pending"},
    {AlertStatus::Firing,   "firing"},
    {AlertStatus::Resolved, "resolved"}
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
    std::string                 created_at;     // 创建时间（字符串）
    std::string                 updated_at;     // 更新时间（字符串）
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
    id, name, description, expression, window, eval_every, severity, tag, selector, for_times, enabled, created_at, updated_at)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertState,
    fingerprint, rule_id, status, severity, labels,
    first_firing_ms, last_eval_ms, last_change_ms, notify_cooldown_ms,
    occurrences, acked, acked_by, acked_at_ms)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEvent,
    timestamp_ms, resolved_timestamp_ms, fingerprint, rule_id, action, status, severity,
    labels, title, description, value, unit, context)

// --------------------
// 面向用户返回的告警事件视图
// --------------------

struct AlertEventAnnotations {
    std::string description;
    std::string summary;
};

struct UserAlertEventView {
    AlertEventAnnotations annotations;
    std::string           created_at;   // YYYY-MM-DD HH:MM:SS
    std::string           ends_at;      // YYYY-MM-DD HH:MM:SS 或空串
    std::string           fingerprint;
    std::string           id;           // 暂时使用 fingerprint 作为 id
    LabelSet              labels;
    std::string           starts_at;    // YYYY-MM-DD HH:MM:SS
    std::string           status;       // inactive/pending/firing/resolved
    std::string           updated_at;   // YYYY-MM-DD HH:MM:SS
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEventAnnotations,
    description, summary)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserAlertEventView,
    annotations, created_at, ends_at, fingerprint, id, labels, starts_at, status, updated_at)

// --------------------
// 面向用户返回/提交的告警规则视图
// --------------------

struct UserRuleCondition {
    std::string op;    // 对应 JSON 字段 "operator"
    double      threshold = 0.0;
};

inline void to_json(nlohmann::json& j, const UserRuleCondition& c) {
    j = nlohmann::json{{"operator", c.op}, {"threshold", c.threshold}};
}
inline void from_json(const nlohmann::json& j, UserRuleCondition& c) {
    j.at("operator").get_to(c.op);
    j.at("threshold").get_to(c.threshold);
}

struct UserRuleExpression {
    std::vector<UserRuleCondition> conditions;
    std::string                    metric;
    std::string                    stable;
    std::vector<LabelSet>          tags;    // 新增：表达式中的 tags 数组（每个元素为一个对象）
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserRuleExpression,
    conditions, metric, stable, tags)

struct UserAlertRule {
    std::string        alert_name;
    std::string        alert_type;
    std::string        created_at;
    std::string        description;
    bool               enabled = true;
    UserRuleExpression expression;
    std::string        for_duration; // 对应 JSON 字段 "for"
    std::string        id;
    std::string        severity;
    std::string        summary;
    std::string        updated_at;
};

inline void to_json(nlohmann::json& j, const UserAlertRule& r) {
    j = nlohmann::json{
        {"alert_name", r.alert_name},
        {"alert_type", r.alert_type},
        {"created_at", r.created_at},
        {"description", r.description},
        {"enabled", r.enabled},
        {"expression", r.expression},
        {"for", r.for_duration},
        {"id", r.id},
        {"severity", r.severity},
        {"summary", r.summary},
        {"updated_at", r.updated_at}
    };
}
inline void from_json(const nlohmann::json& j, UserAlertRule& r) {
    j.at("alert_name").get_to(r.alert_name);
    r.alert_type = j.value("alert_type", std::string());
    r.created_at = j.value("created_at", std::string());
    r.description = j.value("description", std::string());
    r.enabled = j.value("enabled", true);
    if (j.contains("expression")) j.at("expression").get_to(r.expression);
    r.for_duration = j.value("for", std::string());
    r.id = j.value("id", std::string());
    r.severity = j.value("severity", std::string());
    r.summary = j.value("summary", std::string());
    r.updated_at = j.value("updated_at", std::string());
}

} // namespace alert
} // namespace yw


