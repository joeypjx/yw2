#pragma once

#include "alert_event.h"
#include "../infrastructure/database_query_interface.h"
#include <string>
#include <vector>
#include <memory>

namespace yw {
namespace node {
    class INodeModule;  // 前向声明
}

namespace alert {

// 前向声明
struct QueryResult;
class AlertRule;
struct AlertCondition;

class AlertRuleEvaluator {
public:
    explicit AlertRuleEvaluator(std::shared_ptr<DatabaseQueryInterface> dbInterface,
                             node::INodeModule* nodeModule = nullptr);
    
    std::vector<AlertEvent> evaluateRule(const AlertRule& rule);
    static std::string convertRuleToSQL(const AlertRule& rule);
    std::vector<AlertEvent> convertQueryResultToAlerts(const QueryResult& result, 
                                                        const AlertRule& rule);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    node::INodeModule* nodeModule_;  // 可选的 node 模块，用于获取 box_id 和 slot_id
    
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

} // namespace alert
} // namespace yw
