#include "AlertRuleEvaluator.h"
#include "AlertRule.h"
#include <sstream>
#include <algorithm>
#include <regex>

namespace yw {
namespace alertv2 {

AlertRuleEvaluator::AlertRuleEvaluator(std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : dbInterface_(dbInterface) {}

std::vector<Alert> AlertRuleEvaluator::evaluateRule(const AlertRule& rule) {
    // 1. 将告警规则转换为SQL查询
    std::string sql = convertRuleToSQL(rule);
    
    // 2. 执行查询
    QueryResult result = dbInterface_->executeQuery(sql);
    
    // 3. 将查询结果转换为告警对象
    return convertQueryResultToAlerts(result, rule);
}

std::string AlertRuleEvaluator::convertRuleToSQL(const AlertRule& rule) {
    const auto& expr = rule.getExpression();
    std::string tableName = getTableName(expr.stable);
    
    // 构建标签列字符串
    std::ostringstream tagColumnsList;
    auto tagColumns = getTagColumns(expr.stable);
    for (const auto& column : tagColumns) {
        tagColumnsList << ", " << column;
    }
    
    // 使用子查询获取每个节点（和标签组合）的最新数据
    std::ostringstream sql;
    sql << "SELECT DISTINCT ON (host_ip" << tagColumnsList.str() << ") ";
    sql << "host_ip" << tagColumnsList.str() << ", " << expr.metric;
    sql << " FROM " << tableName;
    sql << " WHERE time >= NOW() - INTERVAL '10 seconds'"; // 查询最近10秒的数据
    
    // 添加WHERE条件
    std::string whereConditions = buildWhereConditions(rule);
    if (!whereConditions.empty()) {
        sql << " AND " << whereConditions;
    }
    
    sql << " ORDER BY host_ip" << tagColumnsList.str() << ", time DESC";
    
    return sql.str();
}

std::vector<Alert> AlertRuleEvaluator::convertQueryResultToAlerts(const QueryResult& result, 
                                                                 const AlertRule& rule) {
    std::vector<Alert> alerts;
    
    for (const auto& row : result.rows) {
        // 检查是否满足告警条件
        double metricValue = row.getDoubleValue(rule.getExpression().metric);
        bool conditionMet = checkAlertConditions(metricValue, rule.getExpression().conditions);
        
        if (!conditionMet) {
            continue; // 跳过不满足条件的行
        }
        
        // 构建标签
        std::unordered_map<std::string, std::string> labels;
        labels["alert_name"] = rule.getAlertName();
        labels["alert_type"] = rule.getAlertType();
        labels["severity"] = rule.getSeverity();
        labels["stable"] = rule.getExpression().stable;
        labels["metric"] = rule.getExpression().metric;
        labels["host_ip"] = row.getValue("host_ip");
        labels["value"] = std::to_string(metricValue);
        
        // 添加标签列的值
        auto tagColumns = getTagColumns(rule.getExpression().stable);
        for (const auto& column : tagColumns) {
            std::string value = row.getValue(column);
            if (!value.empty()) {
                labels[column] = value;
            }
        }
        
        // 构建注释
        std::unordered_map<std::string, std::string> annotations;
        annotations["summary"] = fillPlaceholders(rule.getSummary(), labels);
        annotations["description"] = fillPlaceholders(rule.getDescription(), labels);
        
        // 生成指纹
        std::unordered_map<std::string, std::string> fingerprintTags;
        fingerprintTags["host_ip"] = labels["host_ip"];
        for (const auto& tag : rule.getExpression().tags) {
            for (const auto& tagPair : tag) {
                fingerprintTags[tagPair.first] = tagPair.second;
            }
        }
        std::string fingerprint = Alert::generateFingerprint(rule.getAlertName(), fingerprintTags);
        
        // 创建告警对象，根据for字段决定初始状态
        Alert alert(fingerprint, labels, annotations);
        
        // 检查告警规则的for字段
        std::string forDuration = rule.getFor();
        if (forDuration.empty() || forDuration == "0s" || forDuration == "0m" || forDuration == "0h") {
            // 如果没有设置for字段或为0，直接设为Firing状态
            alert.transitionToFiring();
        } else {
            // 如果设置了for字段，设为Pending状态，等待持续时间检查
            alert.transitionToPending();
        }
        
        alerts.push_back(alert);
    }
    
    return alerts;
}

std::string AlertRuleEvaluator::buildWhereConditions(const AlertRule& rule) {
    std::ostringstream conditions;
    
    const auto& expr = rule.getExpression();
    
    // 构建标签条件
    std::string tagConditions = buildTagConditions(expr.tags);
    if (!tagConditions.empty()) {
        conditions << tagConditions;
    }
    
    // 构建指标条件
    std::string metricConditions = buildMetricConditions(expr.conditions, expr.metric);
    if (!metricConditions.empty()) {
        if (!conditions.str().empty()) {
            conditions << " AND ";
        }
        conditions << metricConditions;
    }
    
    return conditions.str();
}

std::string AlertRuleEvaluator::buildTagConditions(const std::vector<std::unordered_map<std::string, std::string>>& tags) {
    if (tags.empty()) {
        return "";
    }
    
    std::ostringstream conditions;
    bool first = true;
    
    for (const auto& tagGroup : tags) {
        if (!first) {
            conditions << " OR ";
        }
        conditions << "(";
        
        bool firstTag = true;
        for (const auto& tag : tagGroup) {
            if (!firstTag) {
                conditions << " AND ";
            }
            conditions << tag.first << " = '" << tag.second << "'";
            firstTag = false;
        }
        
        conditions << ")";
        first = false;
    }
    
    return conditions.str();
}

std::string AlertRuleEvaluator::buildMetricConditions(const std::vector<AlertCondition>& conditions, 
                                                      const std::string& metric) {
    if (conditions.empty()) {
        return "";
    }
    
    std::ostringstream sql;
    bool first = true;
    
    for (const auto& condition : conditions) {
        if (!first) {
            sql << " AND ";
        }
        // 转换操作符为PostgreSQL格式
        std::string op = condition.operator_;
        if (op == "==") op = "=";
        if (op == "!=") op = "<>";
        
        // 对于alive字段，需要特殊处理类型转换
        if (metric == "alive") {
            sql << metric << "::integer " << op << " " << static_cast<int>(condition.threshold);
        } else {
            sql << metric << " " << op << " " << condition.threshold << "::numeric";
        }
        first = false;
    }
    
    return sql.str();
}

std::string AlertRuleEvaluator::getTableName(const std::string& stable) {
    if (stable == "cpu") return "resource_cpu";
    if (stable == "memory") return "resource_memory";
    if (stable == "disk") return "resource_disk";
    if (stable == "network") return "resource_network";
    if (stable == "gpu") return "resource_gpu";
    if (stable == "alive") return "resource_alive";
    throw std::invalid_argument("Unknown stable: " + stable);
}

std::vector<std::string> AlertRuleEvaluator::getTagColumns(const std::string& stable) {
    if (stable == "cpu") return {};
    if (stable == "memory") return {};
    if (stable == "disk") return {"device", "mount_point"};
    if (stable == "network") return {"interface"};
    if (stable == "gpu") return {"gpu_index"};
    if (stable == "alive") return {};
    return {};
}

std::string AlertRuleEvaluator::fillPlaceholders(const std::string& templateStr, 
                                                const std::unordered_map<std::string, std::string>& labels) {
    std::string result = templateStr;
    
    // 替换 {{key}} 格式的占位符
    std::regex placeholderRegex(R"(\{\{([^}]+)\}\})");
    std::smatch match;
    
    while (std::regex_search(result, match, placeholderRegex)) {
        std::string key = match[1].str();
        auto it = labels.find(key);
        std::string value = (it != labels.end()) ? it->second : "";
        result = std::regex_replace(result, std::regex("\\{\\{" + key + "\\}\\}"), value);
    }
    
    return result;
}

bool AlertRuleEvaluator::checkAlertConditions(double value, const std::vector<AlertCondition>& conditions) {
    for (const auto& condition : conditions) {
        bool conditionMet = false;
        
        if (condition.operator_ == ">") {
            conditionMet = (value > condition.threshold);
        } else if (condition.operator_ == "<") {
            conditionMet = (value < condition.threshold);
        } else if (condition.operator_ == ">=") {
            conditionMet = (value >= condition.threshold);
        } else if (condition.operator_ == "<=") {
            conditionMet = (value <= condition.threshold);
        } else if (condition.operator_ == "=" || condition.operator_ == "==") {
            conditionMet = (value == condition.threshold);
        } else if (condition.operator_ == "!=" || condition.operator_ == "<>") {
            conditionMet = (value != condition.threshold);
        }
        
        if (!conditionMet) {
            return false; // 任何一个条件不满足就返回false
        }
    }
    
    return true; // 所有条件都满足
}

} // namespace alertv2
} // namespace yw
