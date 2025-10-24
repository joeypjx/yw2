#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace yw {
namespace alertv2 {

// 前向声明
class DatabaseQueryInterface;
class AlertRuleEvaluator;
class Alert;

// 告警条件
struct AlertCondition {
    std::string operator_;    // 操作符: >, <, >=, <=, ==, !=
    double threshold;         // 阈值
    
    AlertCondition() = default;
    AlertCondition(const std::string& op, double thresh) 
        : operator_(op), threshold(thresh) {}
};

// 告警表达式
struct AlertExpression {
    std::string stable;                           // 资源类型: cpu, memory, disk, network, gpu, alive
    std::string metric;                           // 指标名称
    std::vector<AlertCondition> conditions;       // 条件列表（所有条件需同时满足）
    std::vector<std::unordered_map<std::string, std::string>> tags;  // 标签匹配列表
    
    AlertExpression() = default;
};

// 告警规则类
class AlertRule {
public:
    AlertRule() = default;
    AlertRule(const std::string& name, const AlertExpression& expr, 
              const std::string& duration, const std::string& severity,
              const std::string& summary, const std::string& description,
              const std::string& alertType);
    
    // 获取器方法
    const std::string& getId() const { return id_; }
    const std::string& getAlertName() const { return alert_name_; }
    const AlertExpression& getExpression() const { return expression_; }
    const std::string& getFor() const { return for_; }
    const std::string& getSeverity() const { return severity_; }
    const std::string& getSummary() const { return summary_; }
    const std::string& getDescription() const { return description_; }
    const std::string& getAlertType() const { return alert_type_; }
    const std::string& getCreatedAt() const { return created_at_; }
    const std::string& getUpdatedAt() const { return updated_at_; }
    bool isEnabled() const { return enabled_; }
    
    // 设置器方法
    void setId(const std::string& id) { id_ = id; }
    void setAlertName(const std::string& name) { alert_name_ = name; }
    void setExpression(const AlertExpression& expr) { expression_ = expr; }
    void setFor(const std::string& duration) { for_ = duration; }
    void setSeverity(const std::string& severity) { severity_ = severity; }
    void setSummary(const std::string& summary) { summary_ = summary; }
    void setDescription(const std::string& description) { description_ = description; }
    void setAlertType(const std::string& alertType) { alert_type_ = alertType; }
    void setCreatedAt(const std::string& createdAt) { created_at_ = createdAt; }
    void setUpdatedAt(const std::string& updatedAt) { updated_at_ = updatedAt; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    // 系统生成方法
    void generateId();
    void setCreatedNow();
    void setUpdatedNow();
    
    // JSON序列化方法
    nlohmann::json toJson() const;
    static AlertRule fromJson(const nlohmann::json& j);
    
    // 验证方法
    bool isValid() const;
    std::string getValidationError() const;
    
    // 字符串表示
    std::string toString() const;
    
    // 告警规则评估方法
    std::vector<Alert> evaluate(std::shared_ptr<DatabaseQueryInterface> dbInterface) const;

private:
    std::string id_;              // 系统生成的唯一ID
    std::string alert_name_;      // 告警规则标识
    AlertExpression expression_;   // 告警表达式
    std::string for_;             // 满足持续时间才产生告警，支持s/m/h单位
    std::string severity_;        // 告警等级
    std::string summary_;         // 告警摘要
    std::string description_;     // 告警详情，支持占位符{{}}
    std::string alert_type_;      // 告警类型
    std::string created_at_;     // 创建时间（ISO格式）
    std::string updated_at_;      // 更新时间（ISO格式）
    bool enabled_ = true;         // 是否启用，默认为true
};

// JSON序列化支持
inline void to_json(nlohmann::json& j, const AlertCondition& c) {
    j = nlohmann::json{
        {"operator", c.operator_},
        {"threshold", c.threshold}
    };
}

inline void from_json(const nlohmann::json& j, AlertCondition& c) {
    j.at("operator").get_to(c.operator_);
    j.at("threshold").get_to(c.threshold);
}

inline void to_json(nlohmann::json& j, const AlertExpression& e) {
    j = nlohmann::json{
        {"stable", e.stable},
        {"metric", e.metric},
        {"conditions", e.conditions},
        {"tags", e.tags}
    };
}

inline void from_json(const nlohmann::json& j, AlertExpression& e) {
    j.at("stable").get_to(e.stable);
    j.at("metric").get_to(e.metric);
    j.at("conditions").get_to(e.conditions);
    if (j.contains("tags")) {
        j.at("tags").get_to(e.tags);
    }
}

inline void to_json(nlohmann::json& j, const AlertRule& r) {
    j = nlohmann::json{
        {"id", r.getId()},
        {"alert_name", r.getAlertName()},
        {"expression", r.getExpression()},
        {"for", r.getFor()},
        {"severity", r.getSeverity()},
        {"summary", r.getSummary()},
        {"description", r.getDescription()},
        {"alert_type", r.getAlertType()},
        {"created_at", r.getCreatedAt()},
        {"updated_at", r.getUpdatedAt()},
        {"enabled", r.isEnabled()}
    };
}

inline void from_json(const nlohmann::json& j, AlertRule& r) {
    r = AlertRule::fromJson(j);
}

} // namespace alertv2
} // namespace yw
