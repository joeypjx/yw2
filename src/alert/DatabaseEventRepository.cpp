#include "DatabaseEventRepository.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>

namespace yw {
namespace alert {

DatabaseEventRepository::DatabaseEventRepository(std::shared_ptr<pqxx::connection> conn)
    : conn_(std::move(conn)) {
    // 不再自动创建表，需要手动执行 alert_event_setup_v2.sql
}

bool DatabaseEventRepository::append(const AlertEvent& event) {
    if (!conn_) return false;

    std::lock_guard<std::mutex> lk(mu_);

    try {
        // 转换 labels 为 JSON 字符串
        std::string labels_json = "{}";
        if (!event.labels.empty()) {
            nlohmann::json labels_obj = event.labels;
            labels_json = labels_obj.dump();
        }

        // 解析时间字符串为 PostgreSQL TIMESTAMPTZ
        // 假设时间格式为 ISO8601 或已经是数据库可识别的格式

        // 检查是否已存在相同的记录（使用 fingerprint + starts_at）
        const std::string check_sql = R"(
            SELECT fingerprint FROM alert_event
            WHERE fingerprint = $1 AND starts_at = $2
        )";

        pqxx::work tx(*conn_);
        pqxx::result check_result = tx.exec_params(check_sql,
            event.fingerprint,
            event.starts_at
        );

        if (!check_result.empty()) {
            // 记录已存在，执行更新
            const std::string update_sql = R"(
                UPDATE alert_event
                SET labels = $1, status = $2, summary = $3, description = $4,
                    ends_at = $5, updated_at = now()
                WHERE fingerprint = $6 AND starts_at = $7
            )";

            tx.exec_params(update_sql,
                labels_json,            // $1: labels
                event.status,           // $2: status
                event.summary,          // $3: summary
                event.description,      // $4: description
                event.ends_at.empty() ? std::nullopt : std::optional<std::string>(event.ends_at),  // $5: ends_at
                event.fingerprint,      // $6: fingerprint
                event.starts_at         // $7: starts_at
            );
        } else {
            // 记录不存在，执行插入
            const std::string insert_sql = R"(
                INSERT INTO alert_event (
                    fingerprint, labels, status, summary, description,
                    starts_at, ends_at
                ) VALUES (
                    $1, $2, $3, $4, $5, $6, $7
                )
            )";

            tx.exec_params(insert_sql,
                event.fingerprint,      // $1: fingerprint
                labels_json,            // $2: labels
                event.status,           // $3: status
                event.summary,          // $4: summary
                event.description,      // $5: description
                event.starts_at,        // $6: starts_at
                event.ends_at.empty() ? std::nullopt : std::optional<std::string>(event.ends_at)  // $7: ends_at
            );
        }

        tx.commit();
        return true;

    } catch (const std::exception& e) {
        spdlog::error("DatabaseEventRepository::append error: {}", e.what());
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
                fingerprint,
                labels,
                status,
                summary,
                description,
                starts_at,
                ends_at,
                created_at,
                updated_at
            FROM alert_event
            WHERE starts_at > now() - )" + interval_sql + R"(
            ORDER BY starts_at DESC
        )";

        pqxx::work tx(*conn_);
        pqxx::result r = tx.exec(sql);
        tx.commit();

        // 转换结果
        for (const auto& row : r) {
            AlertEvent event;

            event.fingerprint = row["fingerprint"].as<std::string>();
            event.status = row["status"].as<std::string>();
            event.summary = row["summary"].is_null() ? "" : row["summary"].as<std::string>();
            event.description = row["description"].is_null() ? "" : row["description"].as<std::string>();
            event.starts_at = row["starts_at"].is_null() ? "" : row["starts_at"].as<std::string>();
            event.ends_at = row["ends_at"].is_null() ? "" : row["ends_at"].as<std::string>();
            event.created_at = row["created_at"].is_null() ? "" : row["created_at"].as<std::string>();
            event.updated_at = row["updated_at"].is_null() ? "" : row["updated_at"].as<std::string>();

            // 解析 labels JSON
            if (!row["labels"].is_null()) {
                try {
                    auto labels_json = nlohmann::json::parse(row["labels"].as<std::string>());
                    for (auto it = labels_json.begin(); it != labels_json.end(); ++it) {
                        if (it.value().is_string()) {
                            event.labels[it.key()] = it.value().get<std::string>();
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Failed to parse labels JSON: {}", e.what());
                }
            }

            events.push_back(std::move(event));
        }

    } catch (const std::exception& e) {
        spdlog::error("DatabaseEventRepository::query error: {}", e.what());
    }

    return events;
}

std::size_t DatabaseEventRepository::countByStatus(AlertStatus status) const {
    if (!conn_) return 0;

    std::lock_guard<std::mutex> lk(mu_);

    try {
        std::string status_str;
        switch (status) {
            case AlertStatus::Pending: status_str = "pending"; break;
            case AlertStatus::Firing: status_str = "firing"; break;
            case AlertStatus::Resolved: status_str = "resolved"; break;
            case AlertStatus::Inactive: default: status_str = "inactive"; break;
        }

        const std::string sql = R"(
            SELECT COUNT(*) AS cnt
            FROM alert_event
            WHERE status = $1
        )";

        pqxx::work tx(*conn_);
        pqxx::result r = tx.exec_params(sql, status_str);
        tx.commit();

        if (!r.empty()) {
            return static_cast<std::size_t>(r[0]["cnt"].as<long long>(0));
        }
    } catch (const std::exception& e) {
        spdlog::error("DatabaseEventRepository::countByStatus error: {}", e.what());
    }

    return 0;
}

bool DatabaseEventRepository::hasEvent(const std::string& fingerprint) const {
    if (!conn_) return false;

    std::lock_guard<std::mutex> lk(mu_);

    try {
        pqxx::work tx(*conn_);
        const std::string sql = "SELECT 1 FROM alert_event WHERE fingerprint = $1 LIMIT 1";
        pqxx::result r = tx.exec_params(sql, fingerprint);
        tx.commit();
        return !r.empty();
    } catch (const std::exception& e) {
        spdlog::error("DatabaseEventRepository::hasEvent error: {}", e.what());
        return false;
    }
}

std::optional<AlertEvent> DatabaseEventRepository::getEvent(const std::string& fingerprint) const {
    if (!conn_) return std::nullopt;

    std::lock_guard<std::mutex> lk(mu_);

    try {
        pqxx::work tx(*conn_);
        const std::string sql = R"(
            SELECT
                fingerprint,
                labels,
                status,
                summary,
                description,
                starts_at,
                ends_at,
                created_at,
                updated_at
            FROM alert_event
            WHERE fingerprint = $1
            LIMIT 1
        )";
        pqxx::result r = tx.exec_params(sql, fingerprint);
        tx.commit();

        if (r.empty()) return std::nullopt;

        const auto& row = r[0];
        AlertEvent event;
        event.fingerprint = row["fingerprint"].as<std::string>();
        
        // 解析 labels JSON
        if (!row["labels"].is_null()) {
            nlohmann::json labels_json = nlohmann::json::parse(row["labels"].as<std::string>());
            event.labels = labels_json.get<LabelSet>();
        }
        
        event.status = row["status"].as<std::string>();
        event.summary = row["summary"].as<std::string>();
        event.description = row["description"].as<std::string>();
        event.starts_at = row["starts_at"].is_null() ? "" : row["starts_at"].as<std::string>();
        event.ends_at = row["ends_at"].is_null() ? "" : row["ends_at"].as<std::string>();
        event.created_at = row["created_at"].is_null() ? "" : row["created_at"].as<std::string>();
        event.updated_at = row["updated_at"].is_null() ? "" : row["updated_at"].as<std::string>();

        return event;
    } catch (const std::exception& e) {
        spdlog::error("DatabaseEventRepository::getEvent error: {}", e.what());
        return std::nullopt;
    }
}

bool DatabaseEventRepository::updateEvent(const AlertEvent& event) {
    if (!conn_) return false;

    std::lock_guard<std::mutex> lk(mu_);

    try {
        pqxx::work tx(*conn_);
        
        // 将 labels 转换为 JSON 字符串
        nlohmann::json labels_json = event.labels;
        std::string labels_str = labels_json.dump();

        const std::string sql = R"(
            UPDATE alert_event SET
                labels = $2,
                status = $3,
                summary = $4,
                description = $5,
                starts_at = $6,
                ends_at = $7,
                updated_at = $8
            WHERE fingerprint = $1
        )";

        pqxx::result r = tx.exec_params(sql,
            event.fingerprint,                    // $1: fingerprint
            labels_str,                          // $2: labels
            event.status,                        // $3: status
            event.summary,                       // $4: summary
            event.description,                   // $5: description
            event.starts_at.empty() ? std::nullopt : std::optional<std::string>(event.starts_at),  // $6: starts_at
            event.ends_at.empty() ? std::nullopt : std::optional<std::string>(event.ends_at),      // $7: ends_at
            event.updated_at                     // $8: updated_at
        );

        tx.commit();
        return r.affected_rows() > 0;

    } catch (const std::exception& e) {
        spdlog::error("DatabaseEventRepository::updateEvent error: {}", e.what());
        return false;
    }
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
