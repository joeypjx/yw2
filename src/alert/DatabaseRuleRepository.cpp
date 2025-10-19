#include "DatabaseRuleRepository.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <spdlog/spdlog.h>

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
                id, alert_name, expression, for_duration, severity, summary, description, alert_type,
                created_at, updated_at
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
                id, alert_name, expression, for_duration, severity, summary, description, alert_type,
                created_at, updated_at
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
                id, alert_name, expression, for_duration, severity, summary, description, alert_type
            ) VALUES (
                $1, $2, $3, $4, $5, $6, $7, $8
            )
            ON CONFLICT (id) DO UPDATE SET
                alert_name = EXCLUDED.alert_name,
                expression = EXCLUDED.expression,
                for_duration = EXCLUDED.for_duration,
                severity = EXCLUDED.severity,
                summary = EXCLUDED.summary,
                description = EXCLUDED.description,
                alert_type = EXCLUDED.alert_type,
                updated_at = now()
        )";

        // 转换 severity 枚举为字符串
        std::string severity_str = parseSeverityToString(rule.severity);

        // 转换 expression 为 JSON 字符串
        nlohmann::json expr_json = rule.expression;
        std::string expression_json = expr_json.dump();

        pqxx::work tx(*conn_);
        tx.exec_params(sql,
            rule.id,                    // $1: id
            rule.alert_name,            // $2: alert_name
            expression_json,            // $3: expression
            rule.for_duration,          // $4: for_duration
            severity_str,               // $5: severity
            rule.summary,               // $6: summary
            rule.description,           // $7: description
            rule.alert_type             // $8: alert_type
        );
        tx.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
        spdlog::error("upsertRule error: {}", e.what());
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
                id              TEXT PRIMARY KEY,
                alert_name      TEXT NOT NULL,
                expression      JSONB NOT NULL,
                for_duration    TEXT NOT NULL,
                severity        TEXT NOT NULL,
                summary         TEXT NOT NULL DEFAULT '',
                description     TEXT NOT NULL DEFAULT '',
                alert_type      TEXT NOT NULL DEFAULT 'resource',
                created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
                updated_at      TIMESTAMPTZ NOT NULL DEFAULT now()
            )
        )";
        
        pqxx::work tx(*conn_);
        tx.exec(create_table_sql);
        
        // 创建索引
        const std::string create_severity_index_sql = R"(
            CREATE INDEX IF NOT EXISTS idx_alert_rule_severity
            ON alert_rule (severity, created_at DESC)
        )";
        tx.exec(create_severity_index_sql);

        const std::string create_alert_type_index_sql = R"(
            CREATE INDEX IF NOT EXISTS idx_alert_rule_alert_type
            ON alert_rule (alert_type, created_at DESC)
        )";
        tx.exec(create_alert_type_index_sql);

        const std::string create_expression_index_sql = R"(
            CREATE INDEX IF NOT EXISTS idx_alert_rule_expression_gin
            ON alert_rule USING GIN (expression)
        )";
        tx.exec(create_expression_index_sql);
        
        tx.commit();
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

Rule DatabaseRuleRepository::parseRuleFromRow(const pqxx::row& row) const {
    Rule rule;

    rule.id = row["id"].as<std::string>();
    rule.alert_name = row["alert_name"].as<std::string>();
    rule.for_duration = row["for_duration"].as<std::string>();
    rule.severity = parseSeverityFromString(row["severity"].as<std::string>());
    rule.summary = row["summary"].is_null() ? "" : row["summary"].as<std::string>();
    rule.description = row["description"].is_null() ? "" : row["description"].as<std::string>();
    rule.alert_type = row["alert_type"].is_null() ? "resource" : row["alert_type"].as<std::string>();
    rule.created_at = row["created_at"].is_null() ? std::string("") : row["created_at"].as<std::string>("");
    rule.updated_at = row["updated_at"].is_null() ? std::string("") : row["updated_at"].as<std::string>("");

    // 解析 expression JSONB
    if (!row["expression"].is_null()) {
        try {
            auto expr_json = nlohmann::json::parse(row["expression"].as<std::string>());
            rule.expression = expr_json.get<Expression>();
        } catch (const std::exception& e) {
            // 忽略 JSON 解析错误，使用默认值
            spdlog::error("Failed to parse expression JSON for rule {}: {}", rule.id, e.what());
        }
    }

    return rule;
}

std::string DatabaseRuleRepository::parseSeverityToString(Severity severity) const {
    switch (severity) {
        case Severity::Info: return "提示";
        case Severity::Warn: return "一般";
        case Severity::Critical: return "严重";
        default: return "一般";
    }
}

Severity DatabaseRuleRepository::parseSeverityFromString(const std::string& severity_str) const {
    if (severity_str == "提示") return Severity::Info;
    if (severity_str == "严重") return Severity::Critical;
    return Severity::Warn; // 默认值
}

} // namespace alert
} // namespace yw
