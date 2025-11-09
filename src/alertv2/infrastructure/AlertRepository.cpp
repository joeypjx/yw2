#include "AlertRepository.h"
#include "DatabaseQueryInterface.h"
#include "../domain/Alert.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace yw {
namespace alertv2 {

DatabaseAlertRepository::DatabaseAlertRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : dbInterface_(dbInterface) {
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
}

bool DatabaseAlertRepository::saveAlert(const Alert& alert) {
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
            
            // 从labels中提取host_ip和alert_rule_id
            std::string hostIp = "";
            std::string alertRuleId = "";
            auto labels = alert.getLabels();
            auto hostIt = labels.find("host_ip");
            if (hostIt != labels.end()) {
                hostIp = hostIt->second;
            }
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
                alertRuleId,
                hostIp
            };
            
            QueryResult result = dbInterface_->executeQuery(sql, params);
            return true;
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("保存告警失败: " + std::string(e.what()));
    }
}

std::shared_ptr<Alert> DatabaseAlertRepository::getAlertById(const std::string& id) {
    try {
        std::string sql = buildSelectSql() + " WHERE id = $1";
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        if (result.empty()) {
            return nullptr;
        }
        
        Alert alert = parseAlertFromQueryResult(result[0]);
        return std::make_shared<Alert>(alert);
    } catch (const std::exception& e) {
        throw std::runtime_error("根据ID获取告警失败: " + std::string(e.what()));
    }
}

std::shared_ptr<Alert> DatabaseAlertRepository::getAlertByFingerprint(const std::string& fingerprint) {
    try {
        std::string sql = buildSelectSql() + " WHERE fingerprint = $1";
        std::vector<std::string> params = {fingerprint};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        if (result.empty()) {
            return nullptr;
        }
        
        Alert alert = parseAlertFromQueryResult(result[0]);
        return std::make_shared<Alert>(alert);
    } catch (const std::exception& e) {
        throw std::runtime_error("根据指纹获取告警失败: " + std::string(e.what()));
    }
}

std::vector<Alert> DatabaseAlertRepository::getAlertsByStatus(const std::string& status) {
    try {
        std::string sql = buildSelectSql() + " WHERE status = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {status};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<Alert> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据状态获取告警失败: " + std::string(e.what()));
    }
}

std::vector<Alert> DatabaseAlertRepository::getAlertsByHostIp(const std::string& hostIp) {
    try {
        std::string sql = buildSelectSql() + " WHERE host_ip = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {hostIp};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<Alert> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据节点IP获取告警失败: " + std::string(e.what()));
    }
}


std::vector<Alert> DatabaseAlertRepository::getAlertsByAlertType(const std::string& alertType) {
    try {
        std::string sql = buildSelectSql() + " WHERE labels->>'alert_type' = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {alertType};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<Alert> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据告警类型获取告警失败: " + std::string(e.what()));
    }
}

std::vector<Alert> DatabaseAlertRepository::getAlertsBySeverity(const std::string& severity) {
    try {
        std::string sql = buildSelectSql() + " WHERE labels->>'severity' = $1 ORDER BY created_at DESC";
        std::vector<std::string> params = {severity};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        
        std::vector<Alert> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("根据严重程度获取告警失败: " + std::string(e.what()));
    }
}

std::vector<Alert> DatabaseAlertRepository::getAlertsExceptPending() {
    try {
        std::string sql = buildSelectSql() + " WHERE status != 'pending' ORDER BY created_at DESC";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        std::vector<Alert> alerts;
        for (const auto& row : result.rows) {
            alerts.push_back(parseAlertFromQueryResult(row));
        }
        
        return alerts;
    } catch (const std::exception& e) {
        throw std::runtime_error("获取除Pending外的告警失败: " + std::string(e.what()));
    }
}

bool DatabaseAlertRepository::deleteAlert(const std::string& id) {
    try {
        std::string sql = buildDeleteSql();
        std::vector<std::string> params = {id};
        
        QueryResult result = dbInterface_->executeQuery(sql, params);
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error("删除告警失败: " + std::string(e.what()));
    }
}

bool DatabaseAlertRepository::alertExists(const std::string& id) {
    try {
        std::string sql = "SELECT COUNT(*) FROM alert WHERE id = $1";
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

size_t DatabaseAlertRepository::getAlertCount() {
    try {
        std::string sql = "SELECT COUNT(*) FROM alert";
        
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

Alert DatabaseAlertRepository::parseAlertFromQueryResult(const QueryRow& row) {
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
        Alert alert(row.getValue("fingerprint"), labels, annotations);
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

std::string DatabaseAlertRepository::buildInsertSql() {
    return R"(
        INSERT INTO alert (
            id, fingerprint, labels, annotations, created_at, starts_at, 
            updated_at, ends_at, status, alert_rule_id, host_ip
        ) VALUES (
            $1, $2, $3, $4, $5, 
            NULLIF($6, '')::timestamp,
            $7, 
            NULLIF($8, '')::timestamp,
            $9, $10, $11
        )
    )";
}

std::string DatabaseAlertRepository::buildUpdateSql() {
    return R"(
        UPDATE alert SET 
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

std::string DatabaseAlertRepository::buildSelectSql() {
    return R"(
        SELECT 
            id, fingerprint, labels, annotations, created_at, starts_at, 
            updated_at, ends_at, status, alert_rule_id, host_ip
        FROM alert
    )";
}

std::string DatabaseAlertRepository::buildDeleteSql() {
    return "DELETE FROM alert WHERE id = $1";
}

} // namespace alertv2
} // namespace yw
