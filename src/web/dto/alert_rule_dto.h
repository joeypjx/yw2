// ============================================================================
// 文件功能描述：
// 告警规则DTO（alert_rule_dto）的头文件，定义告警规则API请求和响应的数据传输对象。
// 主要功能包括：
// 1. 用户告警规则视图定义：定义UserRuleView结构体，用于告警规则API的请求和响应数据
// 2. 告警规则条件定义：定义UserRuleCondition结构体，表示告警规则的操作符和阈值
// 3. JSON序列化：提供JSON序列化和反序列化支持，便于API请求和响应格式化
// 4. 数据转换：在内部告警规则模型和用户友好的格式之间进行转换
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <nlohmann/json.hpp>

namespace yw {
namespace web {

// --------------------
// 面向用户返回/提交的告警规则视图
// --------------------

// 用户告警规则条件（操作符和阈值）
struct UserRuleCondition {
    std::string op;    // 对应 JSON 字段 "operator"
    double      threshold = 0.0;
};

// JSON序列化：将UserRuleCondition转换为JSON
inline void to_json(nlohmann::json& j, const UserRuleCondition& c) {
    j = nlohmann::json{{"operator", c.op}, {"threshold", c.threshold}};
}
// JSON反序列化：从JSON解析UserRuleCondition
inline void from_json(const nlohmann::json& j, UserRuleCondition& c) {
    j.at("operator").get_to(c.op);
    j.at("threshold").get_to(c.threshold);
}

// 用户告警规则表达式（包含条件、指标、表名和标签）
struct UserRuleExpression {
    std::vector<UserRuleCondition> conditions;
    std::string                    metric;
    std::string                    stable;
    std::vector<std::map<std::string, std::string>> tags;    // 新增：表达式中的 tags 数组（每个元素为一个对象）
};

// JSON序列化：将UserRuleExpression转换为JSON
inline void to_json(nlohmann::json& j, const UserRuleExpression& e) {
    j = nlohmann::json{
        {"conditions", e.conditions},
        {"metric", e.metric},
        {"stable", e.stable},
        {"tags", e.tags}
    };
}
// JSON反序列化：从JSON解析UserRuleExpression
inline void from_json(const nlohmann::json& j, UserRuleExpression& e) {
    if (j.contains("conditions")) j.at("conditions").get_to(e.conditions); else e.conditions.clear();
    e.metric = j.value("metric", std::string());
    e.stable = j.value("stable", std::string());
    e.tags = j.value("tags", std::vector<std::map<std::string, std::string>>{});
}

// 用户告警规则完整视图（用于API请求和响应）
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

// JSON序列化：将UserAlertRule转换为JSON
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
// JSON反序列化：从JSON解析UserAlertRule
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

} // namespace web
} // namespace yw

