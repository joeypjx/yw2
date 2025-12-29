// ============================================================================
// 文件功能描述：
// 告警规则仓库（DatabaseAlertRuleRepository）的实现文件，负责告警规则的数据持久化。
// 主要功能包括：
// 1. 规则持久化：将告警规则保存到PostgreSQL数据库（如果已存在则更新，否则插入）
// 2. 规则查询：从数据库查询告警规则（按ID、按启用状态、查询所有等）
// 3. 规则删除：从数据库删除指定的告警规则
// 4. SQL构建：构建INSERT、UPDATE、SELECT、DELETE等SQL语句
// 5. 数据转换：在数据库记录和AlertRule对象之间进行转换
// 6. 错误处理：捕获和记录数据库操作异常，提供友好的错误信息
// ============================================================================

#include "alert_rule_repository.h"
#include "database_query_interface.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace yw {
namespace alert {

// 告警规则仓库构造函数
// dbInterface: 数据库查询接口实例，不能为空
DatabaseAlertRuleRepository::DatabaseAlertRuleRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : dbInterface_(dbInterface) {
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
}

// 保存告警规则（如果已存在则更新，否则插入）
// rule: 要保存的告警规则对象
// 返回: 保存成功返回true，失败抛出异常
bool DatabaseAlertRuleRepository::saveRule(const AlertRule& rule) {
    if (!rule.isValid()) {
        throw std::invalid_argument("告警规则无效: " + rule.getValidationError());
    }
    
    try {
        // 检查规则是否已存在
        if (ruleExists(rule.getId())) {
            // 更新现有规则
            std::string sql = buildUpdateSql();
            nlohmann::json exprJson = rule.getExpression();
            std::vector<std::string> params = {
                rule.getAlertName(),
                exprJson.dump(),
                rule.getFor(),
                rule.getSeverity(),
                rule.getSummary(),
                rule.getDescription(),
                rule.getAlertType(),
                rule.isEnabled() ? "true" : "false",
                rule.getUpdatedAt(),
                rule.getId()
            };
            
            QueryResult result = dbInterface_->executeQuery(sql, params);
            return true;
        } else {
            // 插入新规则
            std::string sql = buildInsertSql();
            nlohmann::json exprJson = rule.getExpression();
            std::vector<std::string> params = {
                rule.getId(),
                rule.getAlertName(),
                exprJson.dump(),
                rule.getFor(),
                rule.getSeverity(),
                rule.getSummary(),
                rule.getDescription(),
                rule.getAlertType(),
                rule.isEnabled() ? "true" : "false",
                rule.getCreatedAt(),
                rule.getUpdatedAt()
            };
            
            QueryResult result = dbInterface_->executeQuery(sql, params);
            return true;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("保存告警规则失败: " + std::string(e.what()));
    }
}

// 根据ID获取告警规则
// id: 告警规则唯一标识符
// 返回: 告警规则对象指针，不存在时返回nullptr，失败抛出异常
std::shared_ptr<AlertRule> DatabaseAlertRuleRepository::getRuleById(const std::string& id) {
    try {
        std::string sql = buildSelectSql() + " WHERE id = $1";
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        if (result.empty()) {
            return nullptr;
        }
        
        AlertRule rule = parseRuleFromQueryResult(result[0]);
        return std::make_shared<AlertRule>(rule);
    } catch (const std::exception& e) {
        throw std::runtime_error("获取告警规则失败: " + std::string(e.what()));
    }
}

// 获取所有启用的告警规则
// 返回: 启用的告警规则列表，按创建时间倒序排列，失败抛出异常
std::vector<AlertRule> DatabaseAlertRuleRepository::getEnabledRules() {
    try {
        std::string sql = buildSelectSql() + " WHERE enabled = true ORDER BY created_at DESC";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        std::vector<AlertRule> rules;
        for (const auto& row : result.rows) {
            rules.push_back(parseRuleFromQueryResult(row));
        }
        
        return rules;
    } catch (const std::exception& e) {
        throw std::runtime_error("获取启用的告警规则失败: " + std::string(e.what()));
    }
}

// 获取所有告警规则（包括启用和未启用的）
// 返回: 所有告警规则列表，按创建时间倒序排列，失败抛出异常
std::vector<AlertRule> DatabaseAlertRuleRepository::getAllRules() {
    try {
        std::string sql = buildSelectSql() + " ORDER BY created_at DESC";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        std::vector<AlertRule> rules;
        for (const auto& row : result.rows) {
            rules.push_back(parseRuleFromQueryResult(row));
        }
        
        return rules;
    } catch (const std::exception& e) {
        throw std::runtime_error("获取所有告警规则失败: " + std::string(e.what()));
    }
}

// 删除告警规则
// id: 要删除的告警规则ID
// 返回: 删除成功返回true，失败抛出异常
bool DatabaseAlertRuleRepository::deleteRule(const std::string& id) {
    try {
        std::string sql = buildDeleteSql();
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("删除告警规则失败: " + std::string(e.what()));
    }
}

// 检查告警规则是否存在
// id: 告警规则ID
// 返回: 存在返回true，不存在返回false，失败抛出异常
bool DatabaseAlertRuleRepository::ruleExists(const std::string& id) {
    try {
        std::string sql = "SELECT COUNT(*) as count FROM alert_rule WHERE id = $1";
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        if (result.empty()) {
            return false;
        }
        
        int count = std::stoi(result[0].getValue("count"));
        return count > 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("检查规则是否存在失败: " + std::string(e.what()));
    }
}

// 从数据库查询结果行解析告警规则对象
// row: 数据库查询结果行
// 返回: 解析后的告警规则对象，失败抛出异常
AlertRule DatabaseAlertRuleRepository::parseRuleFromQueryResult(const QueryRow& row) {
    try {
        std::string jsonStr = row.getValue("expression");
        nlohmann::json jsonData = nlohmann::json::parse(jsonStr);
        
        AlertRule rule;
        rule.setId(row.getValue("id"));
        rule.setAlertName(row.getValue("alert_name"));
        rule.setFor(row.getValue("for_duration"));
        rule.setSeverity(row.getValue("severity"));
        rule.setSummary(row.getValue("summary"));
        rule.setDescription(row.getValue("description"));
        rule.setAlertType(row.getValue("alert_type"));
        rule.setCreatedAt(row.getValue("created_at"));
        rule.setUpdatedAt(row.getValue("updated_at"));
        
        // 解析enabled字段
        std::string enabledStr = row.getValue("enabled");
        spdlog::debug("从数据库解析enabled字段: '{}'", enabledStr);
        
        // 更宽松的布尔值解析
        bool enabled = false;
        if (enabledStr == "true" || enabledStr == "1" || enabledStr == "t" || 
            enabledStr == "TRUE" || enabledStr == "True" || enabledStr == "yes" ||
            enabledStr == "YES" || enabledStr == "Yes" || enabledStr == "on" ||
            enabledStr == "ON" || enabledStr == "On") {
            enabled = true;
        }
        
        spdlog::debug("解析后的enabled值: {}", enabled ? "true" : "false");
        rule.setEnabled(enabled);
        
        // 解析expression
        AlertExpression expr;
        expr.stable = jsonData["stable"];
        expr.metric = jsonData["metric"];
        
        // 解析conditions
        if (jsonData.contains("conditions")) {
            for (const auto& cond : jsonData["conditions"]) {
                AlertCondition condition;
                condition.operator_ = cond["operator"];
                condition.threshold = cond["threshold"];
                expr.conditions.push_back(condition);
            }
        }
        
        // 解析tags
        if (jsonData.contains("tags")) {
            for (const auto& tag : jsonData["tags"]) {
                std::unordered_map<std::string, std::string> tagMap;
                for (auto& [key, value] : tag.items()) {
                    tagMap[key] = value;
                }
                expr.tags.push_back(tagMap);
            }
        }
        
        rule.setExpression(expr);
        
        return rule;
    } catch (const std::exception& e) {
        throw std::runtime_error("解析告警规则失败: " + std::string(e.what()));
    }
}

// 构建插入告警规则的SQL语句
// 返回: INSERT SQL语句字符串
std::string DatabaseAlertRuleRepository::buildInsertSql() {
    return R"(
        INSERT INTO alert_rule (
            id, alert_name, expression, for_duration, severity, 
            summary, description, alert_type, enabled, created_at, updated_at
        ) VALUES (
            $1, $2, $3::jsonb, $4, $5, $6, $7, $8, $9, $10, $11
        )
    )";
}

// 构建更新告警规则的SQL语句
// 返回: UPDATE SQL语句字符串
std::string DatabaseAlertRuleRepository::buildUpdateSql() {
    return R"(
        UPDATE alert_rule SET
            alert_name = $1,
            expression = $2::jsonb,
            for_duration = $3,
            severity = $4,
            summary = $5,
            description = $6,
            alert_type = $7,
            enabled = $8,
            updated_at = $9
        WHERE id = $10
    )";
}

// 构建查询告警规则的SQL语句（SELECT部分）
// 返回: SELECT SQL语句字符串
std::string DatabaseAlertRuleRepository::buildSelectSql() {
    return R"(
        SELECT 
            id, alert_name, expression, for_duration, severity,
            summary, description, alert_type, enabled, created_at, updated_at
        FROM alert_rule
    )";
}

// 构建删除告警规则的SQL语句
// 返回: DELETE SQL语句字符串
std::string DatabaseAlertRuleRepository::buildDeleteSql() {
    return "DELETE FROM alert_rule WHERE id = $1";
}

} // namespace alert
} // namespace yw

