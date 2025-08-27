#pragma once

#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include "alert_services.h"

namespace yw {
namespace alert {

class DatabaseRuleRepository : public IRuleRepository {
public:
    explicit DatabaseRuleRepository(std::shared_ptr<pqxx::connection> conn);
    
    std::vector<Rule> listRules() const override;
    std::optional<Rule> getRule(const std::string& id) const override;
    bool upsertRule(const Rule& rule) override;
    bool deleteRule(const std::string& id) override;

private:
    std::shared_ptr<pqxx::connection> conn_;
    mutable std::mutex mu_;
    
    // 辅助方法
    bool ensureTableExists() const;
    Rule parseRuleFromRow(const pqxx::row& row) const;
    std::string parseSeverityToString(Severity severity) const;
    Severity parseSeverityFromString(const std::string& severity_str) const;
};

} // namespace alert
} // namespace yw
