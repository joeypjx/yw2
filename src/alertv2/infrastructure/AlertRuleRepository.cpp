#include "AlertRuleRepository.h"
#include "DatabaseQueryInterface.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace yw {
namespace alertv2 {

DatabaseAlertRuleRepository::DatabaseAlertRuleRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : dbInterface_(dbInterface) {
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
}

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

std::shared_ptr<AlertRule> DatabaseAlertRuleRepository::getRuleByName(const std::string& alertName) {
    try {
        std::string sql = buildSelectSql() + " WHERE alert_name = $1";
        std::vector<std::string> params = {alertName};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        if (result.empty()) {
            return nullptr;
        }
        
        AlertRule rule = parseRuleFromQueryResult(result[0]);
        return std::make_shared<AlertRule>(rule);
    } catch (const std::exception& e) {
        throw std::runtime_error("根据名称获取告警规则失败: " + std::string(e.what()));
    }
}

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

std::vector<AlertRule> DatabaseAlertRuleRepository::getRulesByType(const std::string& alertType) {
    try {
        std::string sql = buildSelectSql() + " WHERE alert_type = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {alertType};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertRule> rules;
        for (const auto& row : result.rows) {
            rules.push_back(parseRuleFromQueryResult(row));
        }
        
        return rules;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据类型获取告警规则失败: " + std::string(e.what()));
    }
}

std::vector<AlertRule> DatabaseAlertRuleRepository::getRulesBySeverity(const std::string& severity) {
    try {
        std::string sql = buildSelectSql() + " WHERE severity = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {severity};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertRule> rules;
        for (const auto& row : result.rows) {
            rules.push_back(parseRuleFromQueryResult(row));
        }
        
        return rules;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据严重等级获取告警规则失败: " + std::string(e.what()));
    }
}

std::vector<AlertRule> DatabaseAlertRuleRepository::getRulesByResourceType(const std::string& resourceType) {
    try {
        std::string sql = buildSelectSql() + " WHERE expression->>'stable' = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {resourceType};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertRule> rules;
        for (const auto& row : result.rows) {
            rules.push_back(parseRuleFromQueryResult(row));
        }
        
        return rules;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据资源类型获取告警规则失败: " + std::string(e.what()));
    }
}

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

bool DatabaseAlertRuleRepository::ruleExists(const std::string& id) {
    try {
        std::string sql = "SELECT COUNT(*) as count FROM alert_rules WHERE id = $1";
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

size_t DatabaseAlertRuleRepository::getRuleCount() {
    try {
        std::string sql = "SELECT COUNT(*) as count FROM alert_rules";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        if (result.empty()) {
            return 0;
        }
        
        return static_cast<size_t>(std::stoi(result[0].getValue("count")));
    } catch (const std::exception& e) {
        throw std::runtime_error("获取规则总数失败: " + std::string(e.what()));
    }
}

bool DatabaseAlertRuleRepository::saveRules(const std::vector<AlertRule>& rules) {
    try {
        for (const auto& rule : rules) {
            if (!saveRule(rule)) {
                return false;
            }
        }
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("批量保存告警规则失败: " + std::string(e.what()));
    }
}

bool DatabaseAlertRuleRepository::deleteRules(const std::vector<std::string>& ids) {
    try {
        for (const auto& id : ids) {
            if (!deleteRule(id)) {
                return false;
            }
        }
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("批量删除告警规则失败: " + std::string(e.what()));
    }
}

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
        std::cout << "从数据库解析enabled字段: '" << enabledStr << "'" << std::endl;
        
        // 更宽松的布尔值解析
        bool enabled = false;
        if (enabledStr == "true" || enabledStr == "1" || enabledStr == "t" || 
            enabledStr == "TRUE" || enabledStr == "True" || enabledStr == "yes" ||
            enabledStr == "YES" || enabledStr == "Yes" || enabledStr == "on" ||
            enabledStr == "ON" || enabledStr == "On") {
            enabled = true;
        }
        
        std::cout << "解析后的enabled值: " << (enabled ? "true" : "false") << std::endl;
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

std::string DatabaseAlertRuleRepository::buildInsertSql() {
    return R"(
        INSERT INTO alert_rules (
            id, alert_name, expression, for_duration, severity, 
            summary, description, alert_type, enabled, created_at, updated_at
        ) VALUES (
            $1, $2, $3::jsonb, $4, $5, $6, $7, $8, $9, $10, $11
        )
    )";
}

std::string DatabaseAlertRuleRepository::buildUpdateSql() {
    return R"(
        UPDATE alert_rules SET
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

std::string DatabaseAlertRuleRepository::buildSelectSql() {
    return R"(
        SELECT 
            id, alert_name, expression, for_duration, severity,
            summary, description, alert_type, enabled, created_at, updated_at
        FROM alert_rules
    )";
}

std::string DatabaseAlertRuleRepository::buildDeleteSql() {
    return "DELETE FROM alert_rules WHERE id = $1";
}

std::string DatabaseAlertRuleRepository::escapeString(const std::string& str) {
    std::string escaped = str;
    
    // 转义单引号
    size_t pos = 0;
    while ((pos = escaped.find("'", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "''");
        pos += 2;
    }
    
    return escaped;
}

} // namespace alertv2
} // namespace yw

