// ============================================================================
// 文件功能描述：
// 告警规则评估器（AlertRuleEvaluator）的头文件，定义告警规则评估功能的接口。
// 主要功能包括：
// 1. 规则评估：评估告警规则，执行SQL查询并生成告警事件
// 2. SQL转换：将告警规则表达式转换为PostgreSQL SQL查询语句
// 3. 表名映射：将告警表达式中的表名（如resource_cpu、resource_memory）映射为实际的数据库表名
// 4. 标签列映射：根据表名确定标签列（如gpu_index、disk_name等），用于告警分组
// 5. 结果转换：将SQL查询结果转换为告警事件对象，提取标签和注释信息
// 6. IP地址解析：从查询结果中解析节点IP地址，提取机箱号和槽位号
// ============================================================================

#pragma once

#include "alert_event.h"
#include "../infrastructure/database_query_interface.h"
#include <string>
#include <vector>
#include <memory>

namespace yw {
namespace alert {

// 前向声明
struct QueryResult;
class AlertRule;
struct AlertCondition;

// 告警规则评估器，负责将告警规则转换为SQL查询并评估结果
class AlertRuleEvaluator {
public:
    // 构造函数，初始化数据库查询接口
    explicit AlertRuleEvaluator(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    
    // 评估告警规则，执行SQL查询并生成告警事件
    // rule: 要评估的告警规则
    // 返回: 生成的告警事件列表
    std::vector<AlertEvent> evaluateRule(const AlertRule& rule);
    // 将告警规则转换为SQL查询语句
    // rule: 告警规则对象
    // 返回: SQL查询语句字符串
    static std::string convertRuleToSQL(const AlertRule& rule);
    // 将查询结果转换为告警事件列表
    // result: 数据库查询结果
    // rule: 告警规则对象
    // 返回: 告警事件列表
    std::vector<AlertEvent> convertQueryResultToAlerts(const QueryResult& result, 
                                                        const AlertRule& rule);

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    // 构建WHERE条件子句
    static std::string buildWhereConditions(const AlertRule& rule);
    // 构建标签过滤条件
    static std::string buildTagConditions(const std::vector<std::unordered_map<std::string, std::string>>& tags);
    // 构建指标条件（如CPU使用率>80%）
    static std::string buildMetricConditions(const std::vector<AlertCondition>& conditions, 
                                            const std::string& metric);
    // 根据stable名称获取对应的数据库表名
    static std::string getTableName(const std::string& stable);
    // 根据stable名称获取标签列名列表
    static std::vector<std::string> getTagColumns(const std::string& stable);
    // 填充模板字符串中的占位符（如{{host_ip}}）
    static std::string fillPlaceholders(const std::string& templateStr, 
                                       const std::unordered_map<std::string, std::string>& labels);
    // 检查指标值是否满足告警条件
    static bool checkAlertConditions(double value, const std::vector<AlertCondition>& conditions);
};

} // namespace alert
} // namespace yw
