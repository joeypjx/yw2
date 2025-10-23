#pragma once

#include "../domain/AlertRule.h"
#include "DatabaseQueryInterface.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alertv2 {

// 告警规则存储接口抽象类
class AlertRuleRepository {
public:
    virtual ~AlertRuleRepository() = default;
    
    // 保存告警规则（创建或更新）
    virtual bool saveRule(const AlertRule& rule) = 0;
    
    // 根据ID获取告警规则
    virtual std::shared_ptr<AlertRule> getRuleById(const std::string& id) = 0;
    
    // 根据告警名称获取告警规则
    virtual std::shared_ptr<AlertRule> getRuleByName(const std::string& alertName) = 0;
    
    // 获取所有告警规则
    virtual std::vector<AlertRule> getAllRules() = 0;
    
    // 获取所有启用的告警规则
    virtual std::vector<AlertRule> getEnabledRules() = 0;
    
    // 根据告警类型获取规则列表
    virtual std::vector<AlertRule> getRulesByType(const std::string& alertType) = 0;
    
    // 根据严重等级获取规则列表
    virtual std::vector<AlertRule> getRulesBySeverity(const std::string& severity) = 0;
    
    // 根据资源类型获取规则列表
    virtual std::vector<AlertRule> getRulesByResourceType(const std::string& resourceType) = 0;
    
    // 删除告警规则
    virtual bool deleteRule(const std::string& id) = 0;
    
    // 检查规则是否存在
    virtual bool ruleExists(const std::string& id) = 0;
    
    // 获取规则总数
    virtual size_t getRuleCount() = 0;
    
    // 批量保存规则
    virtual bool saveRules(const std::vector<AlertRule>& rules) = 0;
    
    // 批量删除规则
    virtual bool deleteRules(const std::vector<std::string>& ids) = 0;
};

// 基于PostgreSQL的告警规则存储实现
class DatabaseAlertRuleRepository : public AlertRuleRepository {
public:
    explicit DatabaseAlertRuleRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~DatabaseAlertRuleRepository() override = default;
    
    // 实现AlertRuleRepository接口
    bool saveRule(const AlertRule& rule) override;
    std::shared_ptr<AlertRule> getRuleById(const std::string& id) override;
    std::shared_ptr<AlertRule> getRuleByName(const std::string& alertName) override;
    std::vector<AlertRule> getAllRules() override;
    std::vector<AlertRule> getEnabledRules() override;
    std::vector<AlertRule> getRulesByType(const std::string& alertType) override;
    std::vector<AlertRule> getRulesBySeverity(const std::string& severity) override;
    std::vector<AlertRule> getRulesByResourceType(const std::string& resourceType) override;
    bool deleteRule(const std::string& id) override;
    bool ruleExists(const std::string& id) override;
    size_t getRuleCount() override;
    bool saveRules(const std::vector<AlertRule>& rules) override;
    bool deleteRules(const std::vector<std::string>& ids) override;

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    // 辅助方法
    AlertRule parseRuleFromQueryResult(const QueryRow& row);
    std::string buildInsertSql();
    std::string buildUpdateSql();
    std::string buildSelectSql();
    std::string buildDeleteSql();
    std::string escapeString(const std::string& str);
};

} // namespace alertv2
} // namespace yw
