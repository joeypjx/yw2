#pragma once

#include "Alert.h"
#include "../infrastructure/DatabaseQueryInterface.h"
#include <string>
#include <vector>

namespace yw {
namespace alertv2 {

// 前向声明
struct QueryResult;
class AlertRule;
struct AlertCondition;

class AlertRuleEvaluator {
public:
    explicit AlertRuleEvaluator(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    
    std::vector<Alert> evaluateRule(const AlertRule& rule);
    static std::string convertRuleToSQL(const AlertRule& rule);
    static std::vector<Alert> convertQueryResultToAlerts(const QueryResult& result, 
                                                        const AlertRule& rule);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    static std::string buildWhereConditions(const AlertRule& rule);
    static std::string buildTagConditions(const std::vector<std::unordered_map<std::string, std::string>>& tags);
    static std::string buildMetricConditions(const std::vector<AlertCondition>& conditions, 
                                            const std::string& metric);
    static std::string getTableName(const std::string& stable);
    static std::vector<std::string> getTagColumns(const std::string& stable);
    static std::string fillPlaceholders(const std::string& templateStr, 
                                       const std::unordered_map<std::string, std::string>& labels);
    static bool checkAlertConditions(double value, const std::vector<AlertCondition>& conditions);
};

} // namespace alertv2
} // namespace yw
