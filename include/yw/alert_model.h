#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "alert_types.h"

namespace yw {
namespace alert {

// 单个条件
struct Condition {
    std::string                 op;         // 操作符: >, <, >=, <=, ==, !=
    double                      threshold;  // 阈值
};

// 表达式定义
struct Expression {
    std::string                 stable;     // 资源名称，如 cpu, memory, disk, network, gpu
    std::string                 metric;     // 指标名称，如 usage_percent, load_avg_1m
    std::vector<Condition>      conditions; // 条件列表（所有条件需同时满足）
    std::vector<LabelSet>       tags;       // 标签匹配列表（用于过滤目标节点）
};

struct Rule {
    std::string                 id;             // 规则ID
    std::string                 alert_name;     // 告警规则名称
    Expression                  expression;     // 表达式配置
    std::string                 for_duration;   // 持续时间，如 30s, 5m（实际改名为for）
    Severity                    severity = Severity::Warn; // 严重等级
    std::string                 summary;        // 告警摘要
    std::string                 description;    // 告警详细描述模板
    std::string                 alert_type;     // 告警类型，如 resource
    bool                        enabled = true; // 是否启用
    std::string                 created_at;     // 创建时间
    std::string                 updated_at;     // 更新时间
};

struct AlertState {
    std::string                 fingerprint;     // 告警指纹（唯一标识）
    AlertStatus                 status = AlertStatus::Inactive;  // 当前状态
    LabelSet                    labels;          // 标签集合
};

struct AlertEvent {
    std::string                 fingerprint;    // 告警指纹（唯一标识）
    yw::alert::LabelSet         labels;         // 标签集合
    std::string                 status;         // 状态: inactive/pending/firing/resolved
    std::string                 summary;        // 告警摘要
    std::string                 description;    // 告警描述
    std::string                 starts_at;      // 开始时间（ISO格式字符串）
    std::string                 ends_at;        // 结束时间（ISO格式字符串，可为空）
    std::string                 created_at;     // 创建时间
    std::string                 updated_at;     // 更新时间
};

// JSON 序列化定义
inline void to_json(nlohmann::json& j, const Condition& c) {
    j = nlohmann::json{{"operator", c.op}, {"threshold", c.threshold}};
}

inline void from_json(const nlohmann::json& j, Condition& c) {
    j.at("operator").get_to(c.op);
    j.at("threshold").get_to(c.threshold);
}

inline void to_json(nlohmann::json& j, const Expression& e) {
    j = nlohmann::json{
        {"stable", e.stable},
        {"metric", e.metric},
        {"conditions", e.conditions},
        {"tags", e.tags}
    };
}

inline void from_json(const nlohmann::json& j, Expression& e) {
    j.at("stable").get_to(e.stable);
    j.at("metric").get_to(e.metric);
    j.at("conditions").get_to(e.conditions);
    j.at("tags").get_to(e.tags);
}

inline void to_json(nlohmann::json& j, const Rule& r) {
    j = nlohmann::json{
        {"id", r.id},
        {"alert_name", r.alert_name},
        {"expression", r.expression},
        {"for_duration", r.for_duration},
        {"severity", static_cast<int>(r.severity)},
        {"summary", r.summary},
        {"description", r.description},
        {"alert_type", r.alert_type},
        {"enabled", r.enabled},
        {"created_at", r.created_at},
        {"updated_at", r.updated_at}
    };
}

inline void from_json(const nlohmann::json& j, Rule& r) {
    j.at("id").get_to(r.id);
    j.at("alert_name").get_to(r.alert_name);
    j.at("expression").get_to(r.expression);
    j.at("for_duration").get_to(r.for_duration);
    r.severity = static_cast<Severity>(j.value("severity", static_cast<int>(Severity::Warn)));
    j.at("summary").get_to(r.summary);
    j.at("description").get_to(r.description);
    j.at("alert_type").get_to(r.alert_type);
    j.at("enabled").get_to(r.enabled);
    j.at("created_at").get_to(r.created_at);
    j.at("updated_at").get_to(r.updated_at);
}

inline void to_json(nlohmann::json& j, const AlertState& s) {
    j = nlohmann::json{
        {"fingerprint", s.fingerprint},
        {"status", static_cast<int>(s.status)},
        {"labels", s.labels}
    };
}

inline void from_json(const nlohmann::json& j, AlertState& s) {
    j.at("fingerprint").get_to(s.fingerprint);
    s.status = static_cast<AlertStatus>(j.value("status", static_cast<int>(AlertStatus::Inactive)));
    j.at("labels").get_to(s.labels);
}

inline void to_json(nlohmann::json& j, const AlertEvent& e) {
    j = nlohmann::json{
        {"fingerprint", e.fingerprint},
        {"labels", e.labels},
        {"status", e.status},
        {"summary", e.summary},
        {"description", e.description},
        {"starts_at", e.starts_at},
        {"ends_at", e.ends_at},
        {"created_at", e.created_at},
        {"updated_at", e.updated_at}
    };
}

inline void from_json(const nlohmann::json& j, AlertEvent& e) {
    j.at("fingerprint").get_to(e.fingerprint);
    j.at("labels").get_to(e.labels);
    j.at("status").get_to(e.status);
    j.at("summary").get_to(e.summary);
    j.at("description").get_to(e.description);
    j.at("starts_at").get_to(e.starts_at);
    j.at("ends_at").get_to(e.ends_at);
    j.at("created_at").get_to(e.created_at);
    j.at("updated_at").get_to(e.updated_at);
}

} // namespace alert
} // namespace yw
