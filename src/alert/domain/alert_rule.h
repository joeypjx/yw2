#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include "alert/alert_model.h"  // 使用公共接口的 AlertCondition 和 AlertExpression 定义

namespace yw {
namespace alert {

// 前向声明
class DatabaseQueryInterface;
class AlertRuleEvaluator;
class Alert;

// 告警规则类，表示一条告警规则的所有配置信息
// 包括规则名称、告警表达式、持续时间、严重程度、描述等
class AlertRule {
public:
    // 默认构造函数
    AlertRule() = default;
    
    // 构造函数，创建新的告警规则
    // name: 告警名称（唯一标识符）
    // expr: 告警表达式（包含查询条件、指标、表名和标签）
    // duration: 持续时间（如"5m"，表示持续5分钟后才触发告警）
    // severity: 严重程度（如"critical"、"warning"、"info"）
    // summary: 摘要（简短描述）
    // description: 详细描述（可包含{{}}占位符）
    // alertType: 告警类型（如"硬件状态"、"业务链路"、"系统告警"）
    AlertRule(const std::string& name, const AlertExpression& expr, 
              const std::string& duration, const std::string& severity,
              const std::string& summary, const std::string& description,
              const std::string& alertType);
    
    // 获取器方法 - 返回规则的各个属性
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
    
    // 设置器方法 - 设置规则的各个属性
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
    
    // 系统生成方法 - 用于自动生成ID和时间戳
    void generateId();           // 生成随机UUID作为规则ID
    void setCreatedNow();        // 设置创建时间为当前时间
    void setUpdatedNow();        // 设置更新时间为当前时间
    
    // JSON序列化方法 - 用于与前端/数据库交互
    nlohmann::json toJson() const;                  // 将规则转换为JSON对象
    static AlertRule fromJson(const nlohmann::json& j);  // 从JSON对象创建规则
    
    // 验证方法 - 检查规则是否有效
    bool isValid() const;                  // 检查规则是否有效
    std::string getValidationError() const;  // 返回验证错误信息（如果有）

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

// JSON序列化支持（AlertCondition 和 AlertExpression 的序列化在 alert_model.h 中定义）
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

} // namespace alert
} // namespace yw
