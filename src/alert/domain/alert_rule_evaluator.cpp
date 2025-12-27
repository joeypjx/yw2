#include "alert_rule_evaluator.h"
#include "alert_rule.h"
#include "utils/ip_address_utils.h"
#include <sstream>
#include <algorithm>
#include <regex>

namespace yw {
namespace alert {

// 构造函数，初始化告警规则评估器
// dbInterface: 数据库查询接口
AlertRuleEvaluator::AlertRuleEvaluator(std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : dbInterface_(dbInterface) {}

// 评估告警规则，执行SQL查询并生成告警事件
// rule: 要评估的告警规则
// 返回: 生成的告警事件列表（如果规则未启用则返回空列表）
std::vector<AlertEvent> AlertRuleEvaluator::evaluateRule(const AlertRule& rule) {

    // 如果规则未启用，直接返回空列表
    if (rule.isEnabled() == false) {
        return std::vector<AlertEvent>();
    }

    // 1. 将告警规则转换为SQL查询
    std::string sql = convertRuleToSQL(rule);
    
    // 2. 执行查询
    QueryResult result = dbInterface_->executeQuery(sql);
    
    // 3. 将查询结果转换为告警对象
    return convertQueryResultToAlerts(result, rule);
}

// 将告警规则转换为SQL查询语句
// rule: 告警规则对象
// 返回: 生成的SQL查询语句（使用DISTINCT ON获取每个节点和标签组合的最新数据）
std::string AlertRuleEvaluator::convertRuleToSQL(const AlertRule& rule) {
    const auto& expr = rule.getExpression();
    std::string tableName = getTableName(expr.stable);
    
    // 构建标签列字符串（如", gpu_index, disk_name"）
    std::ostringstream tagColumnsList;
    auto tagColumns = getTagColumns(expr.stable);
    for (const auto& column : tagColumns) {
        tagColumnsList << ", " << column;
    }
    
    // 使用DISTINCT ON子查询获取每个节点（和标签组合）的最新数据
    // 例如：SELECT DISTINCT ON (host_ip, gpu_index) host_ip, gpu_index, compute_usage FROM resource_gpu ...
    std::ostringstream sql;
    sql << "SELECT DISTINCT ON (host_ip" << tagColumnsList.str() << ") ";
    sql << "host_ip" << tagColumnsList.str() << ", " << expr.metric;
    sql << " FROM " << tableName;
    sql << " WHERE time >= NOW() - INTERVAL '10 seconds'"; // 查询最近10秒的数据
    
    // 添加WHERE条件（如标签过滤、指标条件等）
    std::string whereConditions = buildWhereConditions(rule);
    if (!whereConditions.empty()) {
        sql << " AND " << whereConditions;
    }
    
    // 按host_ip和标签列排序，时间降序，以获取最新数据
    sql << " ORDER BY host_ip" << tagColumnsList.str() << ", time DESC";
    
    return sql.str();
}

// 将查询结果转换为告警事件列表
// result: 数据库查询结果
// rule: 告警规则对象
// 返回: 生成的告警事件列表（只包含满足告警条件的事件）
std::vector<AlertEvent> AlertRuleEvaluator::convertQueryResultToAlerts(const QueryResult& result, 
                                                                 const AlertRule& rule) {
    std::vector<AlertEvent> alerts;
    
    // 遍历查询结果的每一行
    for (const auto& row : result.rows) {
        // 检查指标值是否满足告警条件
        double metricValue = row.getDoubleValue(rule.getExpression().metric);
        bool conditionMet = checkAlertConditions(metricValue, rule.getExpression().conditions);
        
        if (!conditionMet) {
            continue; // 跳过不满足条件的行
        }
        
        // 构建标签（label），包含告警的基本信息和指标值
        std::unordered_map<std::string, std::string> labels;
        labels["alert_name"] = rule.getAlertName();
        labels["alert_type"] = rule.getAlertType();
        labels["severity"] = rule.getSeverity();
        labels["stable"] = rule.getExpression().stable;
        labels["metric"] = rule.getExpression().metric;
        std::string hostIp = row.getValue("host_ip");
        labels["host_ip"] = hostIp;
        labels["value"] = std::to_string(metricValue);
        
        // 通过 host_ip 获取 box_id 和 slot_id（使用 IPAddressUtils 工具类）
        auto boxSlotInfo = yw::utils::IPAddressUtils::parseHostIP(hostIp);
        if (boxSlotInfo.has_value()) {
            labels["box_id"] = std::to_string(boxSlotInfo->box_id);
            labels["slot_id"] = std::to_string(boxSlotInfo->slot_id);
        }
        
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
        std::string fingerprint = AlertEvent::generateFingerprint(rule.getAlertName(), fingerprintTags);
        
        // 创建告警对象，根据for字段决定初始状态
        AlertEvent alert(fingerprint, labels, annotations);
        
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

// 构建标签过滤条件
// tags: 标签组列表，每个标签组是一个键值对映射
// 返回: SQL条件字符串（格式：(tag1='val1' AND tag2='val2') OR (tag3='val3')）
std::string AlertRuleEvaluator::buildTagConditions(const std::vector<std::unordered_map<std::string, std::string>>& tags) {
    if (tags.empty()) {
        return "";
    }
    
    std::ostringstream conditions;
    bool first = true;
    
    // 遍历每个标签组（标签组之间用OR连接）
    for (const auto& tagGroup : tags) {
        if (!first) {
            conditions << " OR ";
        }
        conditions << "(";
        
        // 遍历标签组内的每个标签（标签之间用AND连接）
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

// 构建指标条件（如CPU使用率>80%）
// conditions: 告警条件列表
// metric: 指标名称
// 返回: SQL条件字符串（格式：metric > 80 AND metric < 100）
std::string AlertRuleEvaluator::buildMetricConditions(const std::vector<AlertCondition>& conditions, 
                                                      const std::string& metric) {
    if (conditions.empty()) {
        return "";
    }
    
    std::ostringstream sql;
    bool first = true;
    
    // 遍历每个条件（条件之间用AND连接）
    for (const auto& condition : conditions) {
        if (!first) {
            sql << " AND ";
        }
        // 转换操作符为PostgreSQL格式
        std::string op = condition.operator_;
        if (op == "==") op = "=";
        if (op == "!=") op = "<>";
        
        // 对于alive字段（布尔类型），需要特殊处理类型转换为整数
        if (metric == "alive") {
            sql << metric << "::integer " << op << " " << static_cast<int>(condition.threshold);
        } else {
            sql << metric << " " << op << " " << condition.threshold << "::numeric";
        }
        first = false;
    }
    
    return sql.str();
}

// 根据stable名称获取对应的数据库表名
// stable: stable名称（如"cpu", "memory", "disk"等）
// 返回: 对应的数据库表名（如"resource_cpu"）
// 异常: 如果stable名称未知则抛出invalid_argument异常
std::string AlertRuleEvaluator::getTableName(const std::string& stable) {
    if (stable == "cpu") return "resource_cpu";
    if (stable == "memory") return "resource_memory";
    if (stable == "disk") return "resource_disk";
    if (stable == "network") return "resource_network";
    if (stable == "gpu") return "resource_gpu";
    if (stable == "alive") return "resource_alive";
    throw std::invalid_argument("Unknown stable: " + stable);
}

// 根据stable名称获取标签列名列表
// stable: stable名称（如"cpu", "memory", "disk"等）
// 返回: 标签列名列表（如disk返回{"device", "mount_point"}）
std::vector<std::string> AlertRuleEvaluator::getTagColumns(const std::string& stable) {
    if (stable == "cpu") return {};
    if (stable == "memory") return {};
    if (stable == "disk") return {"device", "mount_point"};
    if (stable == "network") return {"interface"};
    if (stable == "gpu") return {"gpu_index"};
    if (stable == "alive") return {};
    return {};
}

// 填充模板字符串中的占位符（如{{host_ip}}）
// templateStr: 模板字符串（如"节点{{host_ip}}的CPU使用率过高"）
// labels: 标签键值对映射
// 返回: 填充后的字符串（如"节点192.168.2.101的CPU使用率过高"）
std::string AlertRuleEvaluator::fillPlaceholders(const std::string& templateStr, 
                                                const std::unordered_map<std::string, std::string>& labels) {
    std::string result = templateStr;
    
    // 使用正则表达式替换{{key}}格式的占位符
    std::regex placeholderRegex(R"(\{\{([^}]+)\}\})");
    std::smatch match;
    
    // 循环查找并替换所有占位符
    while (std::regex_search(result, match, placeholderRegex)) {
        std::string key = match[1].str();
        auto it = labels.find(key);
        std::string value = (it != labels.end()) ? it->second : "";
        result = std::regex_replace(result, std::regex("\\{\\{" + key + "\\}\\}"), value);
    }
    
    return result;
}

// 检查指标值是否满足告警条件
// value: 指标值
// conditions: 告警条件列表
// 返回: 如果所有条件都满足返回true，否则返回false
bool AlertRuleEvaluator::checkAlertConditions(double value, const std::vector<AlertCondition>& conditions) {
    // 遍历所有条件，全部满足才返回true
    for (const auto& condition : conditions) {
        bool conditionMet = false;
        
        // 根据操作符检查条件是否满足
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

} // namespace alert
} // namespace yw
