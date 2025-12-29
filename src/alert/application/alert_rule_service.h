// ============================================================================
// 文件功能描述：
// 告警规则服务（AlertRuleService）的头文件，定义告警规则业务逻辑管理的接口。
// 主要功能包括：
// 1. 规则管理：提供告警规则的增删改查功能，同时更新数据库和内存缓存
// 2. 内存缓存：维护所有启用规则的内存缓存，提高评估性能
// 3. 规则重载：提供reloadRules方法，从数据库重新加载所有启用的规则
// 4. 规则验证：验证告警规则的有效性（ID、名称、表达式等）
// 5. 启用状态管理：只加载和缓存启用的规则，禁用的规则不参与评估
// ============================================================================

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
     * @brief 获取所有启用的告警规则（从内存缓存）
     */
    std::vector<AlertRule> getEnabledAlertRules() const;
    
    /**
     * @brief 获取所有告警规则（从数据库，包括启用和未启用的）
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

