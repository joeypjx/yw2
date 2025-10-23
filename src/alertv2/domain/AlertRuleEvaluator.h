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

// 告警规则评估器
class AlertRuleEvaluator {
public:
    explicit AlertRuleEvaluator(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    
    // 评估告警规则，返回匹配的告警列表
    std::vector<Alert> evaluateRule(const AlertRule& rule);
    
    // 将告警规则转换为SQL查询语句
    static std::string convertRuleToSQL(const AlertRule& rule);
    
    // 将查询结果转换为告警对象
    static std::vector<Alert> convertQueryResultToAlerts(const QueryResult& result, 
                                                        const AlertRule& rule);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    // 构建WHERE条件
    static std::string buildWhereConditions(const AlertRule& rule);
    
    // 构建标签过滤条件
    static std::string buildTagConditions(const std::vector<std::unordered_map<std::string, std::string>>& tags);
    
    // 构建指标条件
    static std::string buildMetricConditions(const std::vector<AlertCondition>& conditions, 
                                            const std::string& metric);
    
    // 获取表名
    static std::string getTableName(const std::string& stable);
    
    // 获取标签列名
    static std::vector<std::string> getTagColumns(const std::string& stable);
    
    // 填充占位符
    static std::string fillPlaceholders(const std::string& templateStr, 
                                       const std::unordered_map<std::string, std::string>& labels);
    
    // 检查告警条件是否满足
    static bool checkAlertConditions(double value, const std::vector<AlertCondition>& conditions);
};

} // namespace alertv2
} // namespace yw
