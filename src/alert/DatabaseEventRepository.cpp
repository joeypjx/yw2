#include "DatabaseEventRepository.h"
#include <chrono>
#include <sstream>
#include <algorithm>

namespace yw {
namespace alert {

DatabaseEventRepository::DatabaseEventRepository(std::shared_ptr<pqxx::connection> conn)
    : conn_(std::move(conn)) {
    // 不再自动创建表，需要手动执行 alert_event_setup.sql
}

bool DatabaseEventRepository::append(const AlertEvent& event) {
    if (!conn_) return false;
    
    std::lock_guard<std::mutex> lk(mu_);
    
    try {
        // 准备 SQL 语句
        const std::string sql = R"(
            INSERT INTO alert_event (
                time, fingerprint, rule_id, action, status, severity,
                labels, title, description, value, unit, context
            ) VALUES (
                to_timestamp($1), $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12
            )
        )";
        
        // 转换时间戳（毫秒转秒）
        const auto timestamp_s = event.timestamp_ms / 1000;
        
        // 转换 severity 枚举为字符串
        std::string severity_str;
        switch (event.severity) {
            case Severity::Info: severity_str = "info"; break;
            case Severity::Warn: severity_str = "warn"; break;
            case Severity::Critical: severity_str = "critical"; break;
            default: severity_str = "warn"; break;
        }
        
        // 转换 status 枚举为字符串
        std::string status_str;
        switch (event.status) {
            case AlertStatus::Inactive: status_str = "inactive"; break;
            case AlertStatus::Pending: status_str = "pending"; break;
            case AlertStatus::Firing: status_str = "firing"; break;
            case AlertStatus::Resolved: status_str = "resolved"; break;
            default: status_str = "inactive"; break;
        }
        
        // 转换 labels 为 JSON 字符串
        std::string labels_json = "{}";
        if (!event.labels.empty()) {
            nlohmann::json labels_obj = event.labels;
            labels_json = labels_obj.dump();
        }
        
        // 转换 context 为 JSON 字符串
        std::string context_json = "{}";
        if (!event.context.empty()) {
            context_json = event.context.dump();
        }
        
        // 执行插入
        pqxx::work tx(*conn_);
        tx.exec_params(sql,
            timestamp_s,                    // $1: time
            event.fingerprint,              // $2: fingerprint
            event.rule_id,                  // $3: rule_id
            event.action,                   // $4: action
            status_str,                     // $5: status
            severity_str,                   // $6: severity
            labels_json,                    // $7: labels
            event.title,                    // $8: title
            event.description,              // $9: description
            event.value,                    // $10: value
            event.unit,                     // $11: unit
            context_json                    // $12: context
        );
        tx.commit();
        
        return true;
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
        return false;
    }
}

std::vector<AlertEvent> DatabaseEventRepository::query(const std::string& duration) const {
    std::vector<AlertEvent> events;
    if (!conn_) return events;
    
    std::lock_guard<std::mutex> lk(mu_);
    
    try {
        // 解析时间范围
        std::string interval_sql = parseDurationToInterval(duration);
        
        // 查询 SQL
        const std::string sql = R"(
            SELECT 
                EXTRACT(EPOCH FROM time) * 1000 as timestamp_ms,
                fingerprint, rule_id, action, status, severity,
                labels, title, description, value, unit, context
            FROM alert_event 
            WHERE time > now() - )" + interval_sql + R"(
            ORDER BY time DESC
        )";
        
        pqxx::work tx(*conn_);
        pqxx::result r = tx.exec(sql);
        tx.commit();
        
        // 转换结果
        for (const auto& row : r) {
            AlertEvent event;
            
            // 转换时间戳（秒转毫秒）
            event.timestamp_ms = static_cast<std::int64_t>(row["timestamp_ms"].as<double>());
            event.fingerprint = row["fingerprint"].as<std::string>();
            event.rule_id = row["rule_id"].as<std::string>();
            event.action = row["action"].as<std::string>();
            
            // 转换 status 字符串为枚举
            std::string status_str = row["status"].as<std::string>();
            if (status_str == "pending") event.status = AlertStatus::Pending;
            else if (status_str == "firing") event.status = AlertStatus::Firing;
            else if (status_str == "resolved") event.status = AlertStatus::Resolved;
            else event.status = AlertStatus::Inactive;
            
            // 转换 severity 字符串为枚举
            std::string severity_str = row["severity"].as<std::string>();
            if (severity_str == "info") event.severity = Severity::Info;
            else if (severity_str == "critical") event.severity = Severity::Critical;
            else event.severity = Severity::Warn;
            
            // 解析 labels JSON
            if (!row["labels"].is_null()) {
                try {
                    auto labels_json = nlohmann::json::parse(row["labels"].as<std::string>());
                    for (auto it = labels_json.begin(); it != labels_json.end(); ++it) {
                        if (it.value().is_string()) {
                            event.labels[it.key()] = it.value().get<std::string>();
                        }
                    }
                } catch (...) {
                    // 忽略 JSON 解析错误
                }
            }
            
            event.title = row["title"].as<std::string>();
            event.description = row["description"].as<std::string>();
            event.value = row["value"].is_null() ? 0.0 : row["value"].as<double>();
            event.unit = row["unit"].as<std::string>();
            
            // 解析 context JSON
            if (!row["context"].is_null()) {
                try {
                    event.context = nlohmann::json::parse(row["context"].as<std::string>());
                } catch (...) {
                    // 忽略 JSON 解析错误
                }
            }
            
            events.push_back(std::move(event));
        }
        
    } catch (const std::exception& e) {
        // 记录错误但不抛出异常
    }
    
    return events;
}



std::string DatabaseEventRepository::parseDurationToInterval(const std::string& duration) const {
    if (duration.empty()) return "INTERVAL '1 hour'";
    
    // 支持 1h, 30m, 5s 等格式
    char unit = duration.back();
    std::string num = duration.substr(0, duration.size() - 1);
    
    if (num.empty()) num = "1";
    
    switch (unit) {
        case 's': return "INTERVAL '" + num + " seconds'";
        case 'm': return "INTERVAL '" + num + " minutes'";
        case 'h': return "INTERVAL '" + num + " hours'";
        case 'd': return "INTERVAL '" + num + " days'";
        default: return "INTERVAL '1 hour'";
    }
}

} // namespace alert
} // namespace yw
