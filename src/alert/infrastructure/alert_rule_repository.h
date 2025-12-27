#pragma once

#include "../domain/alert_rule.h"
#include "database_query_interface.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

// 告警规则仓库接口，定义告警规则的持久化操作
class AlertRuleRepository {
public:
    virtual ~AlertRuleRepository() = default;
    
    // 保存告警规则（插入或更新）
    virtual bool saveRule(const AlertRule& rule) = 0;
    // 根据ID获取告警规则
    virtual std::shared_ptr<AlertRule> getRuleById(const std::string& id) = 0;
    // 获取所有启用的告警规则
    virtual std::vector<AlertRule> getEnabledRules() = 0;
    // 删除告警规则
    virtual bool deleteRule(const std::string& id) = 0;
    // 检查告警规则是否存在
    virtual bool ruleExists(const std::string& id) = 0;
};

// 基于数据库的告警规则仓库实现
class DatabaseAlertRuleRepository : public AlertRuleRepository {
public:
    explicit DatabaseAlertRuleRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~DatabaseAlertRuleRepository() override = default;
    
    bool saveRule(const AlertRule& rule) override;
    std::shared_ptr<AlertRule> getRuleById(const std::string& id) override;
    std::vector<AlertRule> getEnabledRules() override;
    bool deleteRule(const std::string& id) override;
    bool ruleExists(const std::string& id) override;

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    // 从查询结果行解析告警规则
    AlertRule parseRuleFromQueryResult(const QueryRow& row);
    // 构建插入SQL语句
    std::string buildInsertSql();
    // 构建更新SQL语句
    std::string buildUpdateSql();
    // 构建查询SQL语句
    std::string buildSelectSql();
    // 构建删除SQL语句
    std::string buildDeleteSql();
};

} // namespace alert
} // namespace yw
