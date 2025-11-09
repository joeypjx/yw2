#pragma once

#include "../domain/AlertRule.h"
#include "DatabaseQueryInterface.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alertv2 {

class AlertRuleRepository {
public:
    virtual ~AlertRuleRepository() = default;
    
    virtual bool saveRule(const AlertRule& rule) = 0;
    virtual std::shared_ptr<AlertRule> getRuleById(const std::string& id) = 0;
    virtual std::vector<AlertRule> getEnabledRules() = 0;
    virtual bool deleteRule(const std::string& id) = 0;
    virtual bool ruleExists(const std::string& id) = 0;
};

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
    
    AlertRule parseRuleFromQueryResult(const QueryRow& row);
    std::string buildInsertSql();
    std::string buildUpdateSql();
    std::string buildSelectSql();
    std::string buildDeleteSql();
};

} // namespace alertv2
} // namespace yw
