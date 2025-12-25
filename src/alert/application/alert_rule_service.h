#pragma once

#include "../domain/alert_rule.h"
#include "../infrastructure/alert_rule_repository.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

/**
 * @brief 告警规则管理服务类
 * 
 * 负责告警规则的增删改查操作，维护内存中的规则缓存
 */
class AlertRuleService {
public:
    explicit AlertRuleService(std::shared_ptr<AlertRuleRepository> alertRuleRepo);
    ~AlertRuleService() = default;
    
    /**
     * @brief 添加告警规则
     */
    bool addAlertRule(const AlertRule& rule);
    
    /**
     * @brief 更新告警规则
     */
    bool updateAlertRule(const AlertRule& rule);
    
    /**
     * @brief 删除告警规则
     */
    bool deleteAlertRule(const std::string& ruleId);
    
    /**
     * @brief 根据ID获取告警规则
     */
    std::shared_ptr<AlertRule> getAlertRuleById(const std::string& ruleId);
    
    /**
     * @brief 获取所有告警规则
     */
    std::vector<AlertRule> getAllAlertRules() const;
    
    /**
     * @brief 重新加载所有启用的规则到内存
     */
    void reloadRules();

private:
    std::shared_ptr<AlertRuleRepository> alertRuleRepo_;
    std::vector<AlertRule> rules_;  // 内存中的告警规则缓存
};

} // namespace alert
} // namespace yw

