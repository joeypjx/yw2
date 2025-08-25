#include "DatabaseRuleRepository.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace yw {
namespace alert {

DatabaseRuleRepository::DatabaseRuleRepository(std::shared_ptr<pqxx::connection> conn)
    : conn_(std::move(conn)) {
    ensureTableExists();
}

std::vector<Rule> DatabaseRuleRepository::listRules() const {
    std::vector<Rule> rules;
    if (!conn_) return rules;
    
    std::lock_guard<std::mutex> lk(mu_);
    
    try {
        if (!ensureTableExists()) return rules;
        
        const std::string sql = R"(
            SELECT 
                id, name, description, expression, time_window, eval_every, severity, 
                selector, for_times, enabled, created_at, updated_at
            FROM alert_rule 
            ORDER BY created_at DESC
        )";
        
        pqxx::work tx(*conn_);
        pqxx::result r = tx.exec(sql);
        tx.commit();
        
        for (const auto& row : r) {
            rules.push_back(parseRuleFromRow(row));
        }
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
    }
    
    return rules;
}

std::optional<Rule> DatabaseRuleRepository::getRule(const std::string& id) const {
    if (!conn_) return std::nullopt;
    
    std::lock_guard<std::mutex> lk(mu_);
    
    try {
        if (!ensureTableExists()) return std::nullopt;
        
        const std::string sql = R"(
            SELECT 
                id, name, description, expression, time_window, eval_every, severity, 
                selector, for_times, enabled, created_at, updated_at
            FROM alert_rule 
            WHERE id = $1
        )";
        
        pqxx::work tx(*conn_);
        pqxx::result r = tx.exec_params(sql, id);
        tx.commit();
        
        if (r.empty()) return std::nullopt;
        
        return parseRuleFromRow(r[0]);
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
        return std::nullopt;
    }
}

bool DatabaseRuleRepository::upsertRule(const Rule& rule) {
    if (!conn_) return false;
    
    std::lock_guard<std::mutex> lk(mu_);
    
    try {
        if (!ensureTableExists()) return false;
        
        // 使用 UPSERT 语法 (INSERT ... ON CONFLICT DO UPDATE)
        const std::string sql = R"(
            INSERT INTO alert_rule (
                id, name, description, expression, time_window, eval_every, severity, 
                selector, for_times, enabled
            ) VALUES (
                $1, $2, $3, $4, $5, $6, $7, $8, $9, $10
            )
            ON CONFLICT (id) DO UPDATE SET
                name = EXCLUDED.name,
                description = EXCLUDED.description,
                expression = EXCLUDED.expression,
                time_window = EXCLUDED.time_window,
                eval_every = EXCLUDED.eval_every,
                severity = EXCLUDED.severity,
                selector = EXCLUDED.selector,
                for_times = EXCLUDED.for_times,
                enabled = EXCLUDED.enabled,
                updated_at = now()
        )";
        
        // 转换 severity 枚举为字符串
        std::string severity_str = parseSeverityToString(rule.severity);
        
        // 转换 selector 为 JSON 字符串
        std::string selector_json = "{}";
        if (!rule.selector.empty()) {
            nlohmann::json selector_obj = rule.selector;
            selector_json = selector_obj.dump();
        }
        
        pqxx::work tx(*conn_);
        tx.exec_params(sql,
            rule.id,                    // $1: id
            rule.name,                  // $2: name
            rule.description,           // $3: description
            rule.expression,            // $4: expression
            rule.window,                // $5: window
            rule.eval_every,            // $6: eval_every
            severity_str,               // $7: severity
            selector_json,              // $8: selector
            rule.for_times,             // $9: for_times
            rule.enabled                // $10: enabled
        );
        tx.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
        std::cout << "upsertRule error: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseRuleRepository::deleteRule(const std::string& id) {
    if (!conn_) return false;
    
    std::lock_guard<std::mutex> lk(mu_);
    
    try {
        if (!ensureTableExists()) return false;
        
        const std::string sql = "DELETE FROM alert_rule WHERE id = $1";
        
        pqxx::work tx(*conn_);
        pqxx::result r = tx.exec_params(sql, id);
        tx.commit();
        
        // 返回是否实际删除了记录
        return r.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
        return false;
    }
}

bool DatabaseRuleRepository::ensureTableExists() const {
    if (!conn_) return false;
    
    try {
        const std::string create_table_sql = R"(
            CREATE TABLE IF NOT EXISTS alert_rule (
                id          TEXT PRIMARY KEY,
                name        TEXT NOT NULL,
                description TEXT NOT NULL DEFAULT '',
                expression  TEXT NOT NULL,
                time_window TEXT NOT NULL,
                eval_every  TEXT NOT NULL,
                severity    TEXT NOT NULL,
                selector    JSONB,
                for_times   INTEGER NOT NULL DEFAULT 1,
                enabled     BOOLEAN NOT NULL DEFAULT true,
                created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
                updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
            )
        )";
        
        pqxx::work tx(*conn_);
        tx.exec(create_table_sql);
        
        // 创建索引
        const std::string create_enabled_index_sql = R"(
            CREATE INDEX IF NOT EXISTS idx_alert_rule_enabled 
            ON alert_rule (enabled, created_at DESC)
        )";
        tx.exec(create_enabled_index_sql);
        
        const std::string create_severity_index_sql = R"(
            CREATE INDEX IF NOT EXISTS idx_alert_rule_severity 
            ON alert_rule (severity, created_at DESC)
        )";
        tx.exec(create_severity_index_sql);
        
        tx.commit();
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

Rule DatabaseRuleRepository::parseRuleFromRow(const pqxx::row& row) const {
    Rule rule;
    
    rule.id = row["id"].as<std::string>();
    rule.name = row["name"].as<std::string>();
    rule.description = row["description"].as<std::string>();
    rule.expression = row["expression"].as<std::string>();
    rule.window = row["time_window"].as<std::string>();
    rule.eval_every = row["eval_every"].as<std::string>();
    rule.severity = parseSeverityFromString(row["severity"].as<std::string>());
    rule.for_times = row["for_times"].as<int>();
    rule.enabled = row["enabled"].as<bool>();
    
    // 解析 selector JSON
    if (!row["selector"].is_null()) {
        try {
            auto selector_json = nlohmann::json::parse(row["selector"].as<std::string>());
            for (auto it = selector_json.begin(); it != selector_json.end(); ++it) {
                if (it.value().is_string()) {
                    rule.selector[it.key()] = it.value().get<std::string>();
                }
            }
        } catch (...) {
            // 忽略 JSON 解析错误
        }
    }
    
    return rule;
}

std::string DatabaseRuleRepository::parseSeverityToString(Severity severity) const {
    switch (severity) {
        case Severity::Info: return "info";
        case Severity::Warn: return "warn";
        case Severity::Critical: return "critical";
        default: return "warn";
    }
}

Severity DatabaseRuleRepository::parseSeverityFromString(const std::string& severity_str) const {
    if (severity_str == "info") return Severity::Info;
    if (severity_str == "critical") return Severity::Critical;
    return Severity::Warn; // 默认值
}

} // namespace alert
} // namespace yw
