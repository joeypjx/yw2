#include "alert_event_repository.h"
#include "database_query_interface.h"
#include "../domain/alert_event.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace yw {
namespace alert {

// 告警事件仓库构造函数
// dbInterface: 数据库查询接口实例，不能为空
DatabaseAlertEventRepository::DatabaseAlertEventRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : dbInterface_(dbInterface) {
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
}

// 保存告警事件（如果已存在则更新，否则插入）
// alert: 要保存的告警事件对象
// 返回: 保存成功返回true，失败抛出异常
bool DatabaseAlertEventRepository::saveAlert(const AlertEvent& alert) {
    try {
        // 检查告警是否已存在
        if (alertExists(alert.getId())) {
            // 更新现有告警
            std::string sql = buildUpdateSql();
            std::vector<std::string> params = {
                alert.getFingerprint(),
                nlohmann::json(alert.getLabels()).dump(),
                nlohmann::json(alert.getAnnotations()).dump(),
                alert.getCreatedAt(),
                alert.getStartsAt().empty() ? "" : alert.getStartsAt(),
                alert.getUpdatedAt(),
                alert.getEndsAt().empty() ? "" : alert.getEndsAt(),
                alert.getStatus() == AlertStatus::Firing ? "firing" : 
                alert.getStatus() == AlertStatus::Pending ? "pending" : "resolved",
                alert.getId()
            };
            
            QueryResult result = dbInterface_->executeQuery(sql, params);
            return true;
        } else {
            // 插入新告警
            std::string sql = buildInsertSql();
            
            // 从labels中提取alert_rule_id
            std::string alertRuleId = "";
            auto labels = alert.getLabels();
            auto ruleIt = labels.find("alert_rule_id");
            if (ruleIt != labels.end()) {
                alertRuleId = ruleIt->second;
            }
            
            std::vector<std::string> params = {
                alert.getId(),
                alert.getFingerprint(),
                nlohmann::json(alert.getLabels()).dump(),
                nlohmann::json(alert.getAnnotations()).dump(),
                alert.getCreatedAt(),
                alert.getStartsAt().empty() ? "" : alert.getStartsAt(),
                alert.getUpdatedAt(),
                alert.getEndsAt().empty() ? "" : alert.getEndsAt(),
                alert.getStatus() == AlertStatus::Firing ? "firing" : 
                alert.getStatus() == AlertStatus::Pending ? "pending" : "resolved",
                alertRuleId
            };
            
            QueryResult result = dbInterface_->executeQuery(sql, params);
            return true;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("保存告警失败: " + std::string(e.what()));
    }
}

// 根据ID获取告警事件
// id: 告警事件唯一标识符
// 返回: 告警事件对象指针，不存在时返回nullptr，失败抛出异常
std::shared_ptr<AlertEvent> DatabaseAlertEventRepository::getAlertById(const std::string& id) {
    try {
        std::string sql = buildSelectSql() + " WHERE id = $1";
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        if (result.empty()) {
            return nullptr;
        }
        
        AlertEvent alert = parseAlertFromQueryResult(result[0]);
        return std::make_shared<AlertEvent>(alert);
    } catch (const std::exception& e) {
        throw std::runtime_error("根据ID获取告警失败: " + std::string(e.what()));
    }
}

// 根据指纹获取最新的告警事件
// fingerprint: 告警指纹（唯一标识）
// 返回: 告警事件对象指针，不存在时返回nullptr，失败抛出异常
std::shared_ptr<AlertEvent> DatabaseAlertEventRepository::getAlertByFingerprint(const std::string& fingerprint) {
    try {
        std::string sql = buildSelectSql() + " WHERE fingerprint = $1 ORDER BY created_at DESC LIMIT 1";
        std::vector<std::string> params = {fingerprint};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        if (result.empty()) {
            return nullptr;
        }
        
        AlertEvent alert = parseAlertFromQueryResult(result[0]);
        return std::make_shared<AlertEvent>(alert);
    } catch (const std::exception& e) {
        throw std::runtime_error("根据指纹获取告警失败: " + std::string(e.what()));
    }
}

// 根据指纹和状态获取告警事件列表
// fingerprint: 告警指纹
// status: 告警状态（pending/firing/resolved）
// 返回: 匹配条件的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) {
    try {
        std::string sql = buildSelectSql() + " WHERE fingerprint = $1 AND status = $2 ORDER BY created_at DESC";
        std::vector<std::string> params = {fingerprint, status};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据指纹和状态获取告警失败: " + std::string(e.what()));
    }
}

// 根据状态获取告警事件列表（限制5000条）
// status: 告警状态（pending/firing/resolved）
// 返回: 匹配状态的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByStatus(const std::string& status) {
    try {
        std::string sql = buildSelectSql() + " WHERE status = $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {status};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据状态获取告警失败: " + std::string(e.what()));
    }
}

// 根据状态和类型获取告警事件列表（限制5000条）
// status: 告警状态（pending/firing/resolved）
// alertType: 告警类型（如"硬件状态"、"业务链路"、"系统告警"）
// 返回: 匹配条件的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByStatusAndType(const std::string& status, const std::string& alertType) {
    try {
        std::string sql = buildSelectSql() + " WHERE status = $1 AND labels->>'alert_type' = $2 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {status, alertType};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据状态和类型获取告警失败: " + std::string(e.what()));
    }
}

// 根据节点IP获取告警事件列表（限制5000条）
// hostIp: 节点IP地址
// 返回: 匹配IP地址的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByHostIp(const std::string& hostIp) {
    try {
        // 从 labels JSON 中查询 host_ip
        std::string sql = buildSelectSql() + " WHERE labels->>'host_ip' = $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {hostIp};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据节点IP获取告警失败: " + std::string(e.what()));
    }
}

// 根据机箱号获取告警事件列表（限制5000条）
// boxId: 机箱编号（1-9）
// 返回: 匹配机箱号的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByBoxId(int boxId) {
    try {
        // 从 labels JSON 中查询 box_id
        std::string sql = buildSelectSql() + " WHERE labels->>'box_id' = $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {std::to_string(boxId)};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据机箱号获取告警失败: " + std::string(e.what()));
    }
}

std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsBySlotId(int slotId) {
    try {
        // 从 labels JSON 中查询 slot_id
        std::string sql = buildSelectSql() + " WHERE labels->>'slot_id' = $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {std::to_string(slotId)};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据板卡号获取告警失败: " + std::string(e.what()));
    }
}

// 根据时间范围获取告警事件列表（限制5000条）
// startTime: 开始时间（PostgreSQL timestamp格式）
// endTime: 结束时间（PostgreSQL timestamp格式）
// 返回: 在指定时间范围内创建的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) {
    try {
        // 根据 created_at 时间范围查询告警
        std::string sql = buildSelectSql() + " WHERE created_at >= $1::timestamp AND created_at <= $2::timestamp ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {startTime, endTime};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据时间范围获取告警失败: " + std::string(e.what()));
    }
}

// 根据告警类型获取告警事件列表（限制5000条）
// alertType: 告警类型（如"硬件状态"、"业务链路"、"系统告警"）
// 返回: 匹配类型的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByAlertType(const std::string& alertType) {
    try {
        std::string sql = buildSelectSql() + " WHERE labels->>'alert_type' = $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {alertType};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据告警类型获取告警失败: " + std::string(e.what()));
    }
}

// 根据严重程度获取告警事件列表（限制5000条）
// severity: 严重程度（如"critical"、"warning"、"info"）
// 返回: 匹配严重程度的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsBySeverity(const std::string& severity) {
    try {
        std::string sql = buildSelectSql() + " WHERE labels->>'severity' = $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {severity};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据严重程度获取告警失败: " + std::string(e.what()));
    }
}

// 根据描述内容获取告警事件列表（模糊匹配，限制5000条）
// description: 描述关键词，支持LIKE模糊匹配
// 返回: 描述中包含关键词的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByDescription(const std::string& description) {
    try {
        // 从 annotations JSON 中查询 description，支持模糊匹配
        std::string sql = buildSelectSql() + " WHERE annotations->>'description' LIKE $1 ORDER BY created_at DESC LIMIT 5000";
        std::vector<std::string> params = {"%" + description + "%"};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据描述获取告警失败: " + std::string(e.what()));
    }
}

// 获取除Pending状态外的所有告警事件（即Firing和Resolved状态，限制5000条）
// 返回: Firing和Resolved状态的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsExceptPending() {
    try {
        std::string sql = buildSelectSql() + " WHERE status IN ('firing', 'resolved') ORDER BY created_at DESC LIMIT 5000";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("获取除Pending外的告警失败: " + std::string(e.what()));
    }
}

// 根据多个过滤条件组合查询告警事件
// filters: 过滤条件对象，包含状态、类型、时间范围等多个条件
// 返回: 匹配所有过滤条件的告警事件列表，失败抛出异常
std::vector<AlertEvent> DatabaseAlertEventRepository::getAlertsByFilters(const AlertFilters& filters) {
    try {
        // 构建 WHERE 子句和参数
        auto [whereClause, params] = buildWhereClause(filters);
        
        // 构建完整 SQL
        std::string sql = buildSelectSql();
        if (!whereClause.empty()) {
            sql += " WHERE " + whereClause;
        }
        sql += " ORDER BY created_at DESC";
        
        // 添加 LIMIT
        int limit = filters.limit;
        if (limit <= 0) limit = 100;
        if (limit > 1000) limit = 1000;
        sql += " LIMIT " + std::to_string(limit);
        
        // 执行查询
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<AlertEvent> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据过滤条件获取告警失败: " + std::string(e.what()));
    }
}

bool DatabaseAlertEventRepository::deleteAlert(const std::string& id) {
    try {
        std::string sql = buildDeleteSql();
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("删除告警失败: " + std::string(e.what()));
    }
}

// 删除指定指纹和状态的所有告警事件
// fingerprint: 告警指纹
// status: 告警状态（pending/firing/resolved）
// 返回: 删除的告警数量，失败抛出异常
int DatabaseAlertEventRepository::deleteAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) {
    try {
        // 先查询要删除的数量
        std::string countSql = "SELECT COUNT(*) as count FROM alert_event WHERE fingerprint = $1 AND status = $2";
        std::vector<std::string> params = {fingerprint, status};
        QueryResult countResult = dbInterface_->executeQuery(countSql, params);
        
        int count = 0;
        if (!countResult.rows.empty()) {
            std::string countStr = countResult.rows[0].getValue("count");
            count = std::stoi(countStr);
        }
        
        // 如果有要删除的告警，执行删除
        if (count > 0) {
            std::string sql = "DELETE FROM alert_event WHERE fingerprint = $1 AND status = $2";
            dbInterface_->executeQuery(sql, params);
        }
        
        return count;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据指纹和状态删除告警失败: " + std::string(e.what()));
    }
}

// 将指定指纹的所有Firing状态告警标记为Resolved
// fingerprint: 告警指纹
// 返回: 更新的告警数量，失败抛出异常
int DatabaseAlertEventRepository::resolveFiringAlertsByFingerprint(const std::string& fingerprint) {
    try {
        // 先查询要更新的数量
        std::string countSql = "SELECT COUNT(*) as count FROM alert_event WHERE fingerprint = $1 AND status = $2";
        std::vector<std::string> params = {fingerprint, "firing"};
        QueryResult countResult = dbInterface_->executeQuery(countSql, params);
        
        int count = 0;
        if (!countResult.rows.empty()) {
            std::string countStr = countResult.rows[0].getValue("count");
            count = std::stoi(countStr);
        }
        
        // 如果有要更新的告警，执行更新：将status改为resolved，设置ends_at为当前时间，更新updated_at
        if (count > 0) {
            std::string sql = R"(
                UPDATE alert_event SET 
                    status = 'resolved',
                    ends_at = NOW(),
                    updated_at = NOW()
                WHERE fingerprint = $1 AND status = $2
            )";
            dbInterface_->executeQuery(sql, params);
        }
        
        return count;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据指纹将firing告警标记为resolved失败: " + std::string(e.what()));
    }
}

// 检查告警事件是否存在
// id: 告警事件ID
// 返回: 存在返回true，不存在返回false，失败抛出异常
bool DatabaseAlertEventRepository::alertExists(const std::string& id) {
    try {
        std::string sql = "SELECT COUNT(*) FROM alert_event WHERE id = $1";
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        if (!result.empty()) {
            std::string countStr = result[0].getValue("count");
            return std::stoi(countStr) > 0;
        }
        return false;
    } catch (const std::exception& e) {
        throw std::runtime_error("检查告警是否存在失败: " + std::string(e.what()));
    }
}

// 获取告警事件总数
// 返回: 告警事件总数，失败抛出异常
size_t DatabaseAlertEventRepository::getAlertCount() {
    try {
        std::string sql = "SELECT COUNT(*) FROM alert_event";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        if (!result.empty()) {
            std::string countStr = result[0].getValue("count");
            return std::stoi(countStr);
        }
        return 0;
    } catch (const std::exception& e) {
        throw std::runtime_error("获取告警总数失败: " + std::string(e.what()));
    }
}

// 从数据库查询结果行解析告警事件对象
// row: 数据库查询结果行
// 返回: 解析后的告警事件对象，失败抛出异常
AlertEvent DatabaseAlertEventRepository::parseAlertFromQueryResult(const QueryRow& row) {
    try {
        // 解析labels
        std::string labelsStr = row.getValue("labels");
        nlohmann::json labelsJson = nlohmann::json::parse(labelsStr);
        std::unordered_map<std::string, std::string> labels;
        for (auto& [key, value] : labelsJson.items()) {
            labels[key] = value;
        }
        
        // 解析annotations
        std::string annotationsStr = row.getValue("annotations");
        nlohmann::json annotationsJson = nlohmann::json::parse(annotationsStr);
        std::unordered_map<std::string, std::string> annotations;
        for (auto& [key, value] : annotationsJson.items()) {
            annotations[key] = value;
        }
        
        // 创建告警对象
        AlertEvent alert(row.getValue("fingerprint"), labels, annotations);
        alert.setId(row.getValue("id"));
        alert.setCreatedAt(row.getValue("created_at"));
        alert.setStartsAt(row.getValue("starts_at"));
        alert.setUpdatedAt(row.getValue("updated_at"));
        alert.setEndsAt(row.getValue("ends_at"));
        
        // 设置状态
        std::string statusStr = row.getValue("status");
        if (statusStr == "firing") {
            alert.setStatus(AlertStatus::Firing);
        } else if (statusStr == "pending") {
            alert.setStatus(AlertStatus::Pending);
        } else if (statusStr == "resolved") {
            alert.setStatus(AlertStatus::Resolved);
        }
        
        return alert;
    } catch (const std::exception& e) {
        throw std::runtime_error("解析告警失败: " + std::string(e.what()));
    }
}

// 构建插入告警事件的SQL语句
// 返回: INSERT SQL语句字符串
std::string DatabaseAlertEventRepository::buildInsertSql() {
    return R"(
        INSERT INTO alert_event (
            id, fingerprint, labels, annotations, created_at, starts_at, 
            updated_at, ends_at, status, alert_rule_id
        ) VALUES (
            $1, $2, $3, $4, $5, 
            NULLIF($6, '')::timestamp,
            $7, 
            NULLIF($8, '')::timestamp,
            $9, $10
        )
    )";
}

// 构建更新告警事件的SQL语句
// 返回: UPDATE SQL语句字符串
std::string DatabaseAlertEventRepository::buildUpdateSql() {
    return R"(
        UPDATE alert_event SET 
            fingerprint = $1,
            labels = $2,
            annotations = $3,
            created_at = $4,
            starts_at = NULLIF($5, '')::timestamp,
            updated_at = $6,
            ends_at = NULLIF($7, '')::timestamp,
            status = $8
        WHERE id = $9
    )";
}

// 构建查询告警事件的SQL语句（SELECT部分）
// 返回: SELECT SQL语句字符串
std::string DatabaseAlertEventRepository::buildSelectSql() {
    return R"(
        SELECT 
            id, fingerprint, labels, annotations, created_at, starts_at, 
            updated_at, ends_at, status, alert_rule_id
        FROM alert_event
    )";
}

// 构建删除告警事件的SQL语句
// 返回: DELETE SQL语句字符串
std::string DatabaseAlertEventRepository::buildDeleteSql() {
    return "DELETE FROM alert_event WHERE id = $1";
}

// 根据过滤条件构建WHERE子句和参数列表
// filters: 过滤条件对象
// 返回: (WHERE子句字符串, 参数列表) 的pair
std::pair<std::string, std::vector<std::string>> DatabaseAlertEventRepository::buildWhereClause(const AlertFilters& filters) {
    std::vector<std::string> conditions;
    std::vector<std::string> params;
    int paramIndex = 1;
    
    // 状态过滤
    if (!filters.status.empty()) {
        conditions.push_back("status = $" + std::to_string(paramIndex++));
        params.push_back(filters.status);
    }
    
    // 严重程度过滤
    if (!filters.severity.empty()) {
        conditions.push_back("labels->>'severity' = $" + std::to_string(paramIndex++));
        params.push_back(filters.severity);
    }
    
    // 告警类型过滤
    if (!filters.alert_type.empty()) {
        conditions.push_back("labels->>'alert_type' = $" + std::to_string(paramIndex++));
        params.push_back(filters.alert_type);
    }
    
    // 主机IP过滤
    if (!filters.host_ip.empty()) {
        conditions.push_back("labels->>'host_ip' = $" + std::to_string(paramIndex++));
        params.push_back(filters.host_ip);
    }
    
    // 机箱号过滤
    if (filters.box_id >= 0) {
        conditions.push_back("labels->>'box_id' = $" + std::to_string(paramIndex++));
        params.push_back(std::to_string(filters.box_id));
    }
    
    // 板卡号过滤
    if (filters.slot_id >= 0) {
        conditions.push_back("labels->>'slot_id' = $" + std::to_string(paramIndex++));
        params.push_back(std::to_string(filters.slot_id));
    }
    
    // 时间范围过滤
    if (!filters.start_time.empty()) {
        conditions.push_back("created_at >= $" + std::to_string(paramIndex++) + "::timestamp");
        params.push_back(filters.start_time);
    }
    if (!filters.end_time.empty()) {
        conditions.push_back("created_at <= $" + std::to_string(paramIndex++) + "::timestamp");
        params.push_back(filters.end_time);
    }
    
    // 描述过滤（模糊匹配）
    if (!filters.description.empty()) {
        conditions.push_back("annotations->>'description' LIKE $" + std::to_string(paramIndex++));
        params.push_back("%" + filters.description + "%");
    }
    
    // 栈名过滤
    if (!filters.stack_name.empty()) {
        conditions.push_back("labels->>'stack_name' = $" + std::to_string(paramIndex++));
        params.push_back(filters.stack_name);
    }
    
    // 组件名过滤
    if (!filters.component_name.empty()) {
        conditions.push_back("labels->>'component_name' = $" + std::to_string(paramIndex++));
        params.push_back(filters.component_name);
    }
    
    // 如果没有过滤条件，返回空字符串
    if (conditions.empty()) {
        return {"", params};
    }
    
    // 组合所有条件
    std::string whereClause = conditions[0];
    for (size_t i = 1; i < conditions.size(); ++i) {
        whereClause += " AND " + conditions[i];
    }
    
    return {whereClause, params};
}

} // namespace alert
} // namespace yw
