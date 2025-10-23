// 先包含third_party/json，确保使用正确的版本
#include "../../../third_party/json/include/nlohmann/json.hpp"

// 然后包含其他头文件
#include "AlertV2Routes.h"
#include "../../alertv2/application/AlertEngine.h"
#include "../../alertv2/domain/AlertRule.h"
#include <iostream>

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerAlertV2Routes(hv::HttpService* service, 
                          std::shared_ptr<yw::alertv2::AlertEngine> alertEngine) {
    if (!service || !alertEngine) return;

    // POST /api/v2/alarm/rules - 创建告警规则
    service->POST("/api/v2/alarm/rules", [alertEngine](const HttpContextPtr& ctx) {
        try {
            auto body = ctx->body();
            if (body.empty()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "empty request body"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }

            // 直接使用AlertRule的fromJson方法
            auto ruleJson = json::parse(body);
            yw::alertv2::AlertRule rule = yw::alertv2::AlertRule::fromJson(ruleJson);
            
            // 验证规则
            if (!rule.isValid()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "invalid rule: " + rule.getValidationError()}, 
                           {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }

            // 使用AlertEngine保存规则
            bool success = alertEngine->addAlertRule(rule);
            if (success) {
                json resp = {{"api_version", 2}, {"status", "success"}, 
                           {"data", {{"id", rule.getId()}, {"message", "Rule created successfully"}}}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            } else {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "failed to create rule"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }

        } catch (const json::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "invalid JSON format: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/rules - 获取所有告警规则
    service->GET("/api/v2/alarm/rules", [alertEngine](const HttpContextPtr& ctx) {
        try {
            auto rules = alertEngine->getAllAlertRules();
            json rulesArray = json::array();
            
            for (const auto& rule : rules) {
                // 直接使用AlertRule的toJson方法
                rulesArray.push_back(rule.toJson());
            }
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", rulesArray}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/rules/{id} - 获取特定告警规则
    service->GET("/api/v2/alarm/rules/{id}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            auto rule = alertEngine->getAlertRuleById(ruleId);
            
            if (!rule) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "rule not found"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_NOT_FOUND);
                return ctx->send(resp.dump(2));
            }

            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", rule->toJson()}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // PUT /api/v2/alarm/rules/{id} - 更新告警规则
    service->PUT("/api/v2/alarm/rules/{id}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            auto body = ctx->body();
            
            if (body.empty()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "empty request body"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }

            // 直接使用AlertRule的fromJson方法
            auto ruleJson = json::parse(body);
            yw::alertv2::AlertRule rule = yw::alertv2::AlertRule::fromJson(ruleJson);
            
            // 确保ID匹配
            rule.setId(ruleId);
            
            // 验证规则
            if (!rule.isValid()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "invalid rule: " + rule.getValidationError()}, 
                           {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }

            // 使用AlertEngine更新规则
            bool success = alertEngine->updateAlertRule(rule);
            if (success) {
                json resp = {{"api_version", 2}, {"status", "success"}, 
                           {"data", {{"id", rule.getId()}, {"message", "Rule updated successfully"}}}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            } else {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "failed to update rule"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }

        } catch (const json::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "invalid JSON format: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // POST /api/v2/alarm/rules/{id}/delete - 删除告警规则
    service->POST("/api/v2/alarm/rules/{id}/delete", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            bool success = alertEngine->deleteAlertRule(ruleId);
            
            if (success) {
                json resp = {{"api_version", 2}, {"status", "success"}, 
                           {"data", {{"id", ruleId}, {"message", "Rule deleted successfully"}}}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            } else {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "failed to delete rule"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/rules/enabled - 获取启用的告警规则
    service->GET("/api/v2/alarm/rules/enabled", [alertEngine](const HttpContextPtr& ctx) {
        try {
            // 这里需要AlertEngine提供getEnabledRules方法
            // 暂时使用getAllAlertRules然后过滤
            auto allRules = alertEngine->getAllAlertRules();
            json enabledRulesArray = json::array();
            
            for (const auto& rule : allRules) {
                if (rule.isEnabled()) {
                    enabledRulesArray.push_back(rule.toJson());
                }
            }
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", enabledRulesArray}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // POST /api/v2/alarm/rules/{id}/enable - 启用告警规则
    service->POST("/api/v2/alarm/rules/{id}/enable", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            auto rule = alertEngine->getAlertRuleById(ruleId);
            
            if (!rule) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "rule not found"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_NOT_FOUND);
                return ctx->send(resp.dump(2));
            }

            // 创建副本并启用
            yw::alertv2::AlertRule updatedRule = *rule;
            updatedRule.setEnabled(true);
            updatedRule.setUpdatedNow();
            
            bool success = alertEngine->updateAlertRule(updatedRule);
            if (success) {
                json resp = {{"api_version", 2}, {"status", "success"}, 
                           {"data", {{"id", ruleId}, {"message", "Rule enabled successfully"}}}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            } else {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "failed to enable rule"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // POST /api/v2/alarm/rules/{id}/disable - 禁用告警规则
    service->POST("/api/v2/alarm/rules/{id}/disable", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            auto rule = alertEngine->getAlertRuleById(ruleId);
            
            if (!rule) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "rule not found"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_NOT_FOUND);
                return ctx->send(resp.dump(2));
            }

            // 创建副本并禁用
            yw::alertv2::AlertRule updatedRule = *rule;
            updatedRule.setEnabled(false);
            updatedRule.setUpdatedNow();
            
            bool success = alertEngine->updateAlertRule(updatedRule);
            if (success) {
                json resp = {{"api_version", 2}, {"status", "success"}, 
                           {"data", {{"id", ruleId}, {"message", "Rule disabled successfully"}}}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            } else {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "failed to disable rule"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/events - 获取告警事件
    service->GET("/api/v2/alarm/events", [alertEngine](const HttpContextPtr& ctx) {
        try {
            // 获取查询参数
            auto params = ctx->params();
            
            // 默认获取最近24小时的告警
            std::string duration = "24h";
            if (params.find("duration") != params.end()) {
                duration = params["duration"];
            }
            
            // 获取状态过滤参数
            std::string statusFilter;
            if (params.find("status") != params.end()) {
                statusFilter = params["status"];
            }
            
            // 获取严重程度过滤参数
            std::string severityFilter;
            if (params.find("severity") != params.end()) {
                severityFilter = params["severity"];
            }
            
            // 获取告警类型过滤参数
            std::string alertTypeFilter;
            if (params.find("alert_type") != params.end()) {
                alertTypeFilter = params["alert_type"];
            }
            
            // 获取主机IP过滤参数
            std::string hostIpFilter;
            if (params.find("host_ip") != params.end()) {
                hostIpFilter = params["host_ip"];
            }
            
            // 获取限制数量参数
            int limit = 100; // 默认限制100条
            if (params.find("limit") != params.end()) {
                try {
                    limit = std::stoi(params["limit"]);
                    if (limit <= 0) limit = 100;
                    if (limit > 1000) limit = 1000; // 最大限制1000条
                } catch (...) {
                    limit = 100;
                }
            }
            
            std::vector<yw::alertv2::Alert> alerts;
            
            // 根据过滤条件获取告警
            if (!statusFilter.empty()) {
                alerts = alertEngine->getAlertsByStatus(statusFilter);
            } else if (!severityFilter.empty()) {
                alerts = alertEngine->getAlertsBySeverity(severityFilter);
            } else if (!alertTypeFilter.empty()) {
                alerts = alertEngine->getAlertsByAlertType(alertTypeFilter);
            } else if (!hostIpFilter.empty()) {
                alerts = alertEngine->getAlertsByHostIp(hostIpFilter);
            } else {
                // 获取最近告警
                alerts = alertEngine->getRecentAlerts(limit);
            }
            
            // 构建响应数据
            json alertsArray = json::array();
            
            for (const auto& alert : alerts) {
                json alertJson;
                alertJson["id"] = alert.getId();
                alertJson["fingerprint"] = alert.getFingerprint();
                
                // 状态转换
                std::string statusStr;
                switch (alert.getStatus()) {
                    case yw::alertv2::AlertStatus::Pending:
                        statusStr = "pending";
                        break;
                    case yw::alertv2::AlertStatus::Firing:
                        statusStr = "firing";
                        break;
                    case yw::alertv2::AlertStatus::Resolved:
                        statusStr = "resolved";
                        break;
                    default:
                        statusStr = "unknown";
                        break;
                }
                alertJson["status"] = statusStr;
                
                // 时间字段
                alertJson["created_at"] = alert.getCreatedAt();
                alertJson["updated_at"] = alert.getUpdatedAt();
                alertJson["starts_at"] = alert.getStartsAt();
                alertJson["ends_at"] = alert.getEndsAt();
                
                // 标签
                alertJson["labels"] = json::object();
                auto labels = alert.getLabels();
                for (const auto& label : labels) {
                    alertJson["labels"][label.first] = label.second;
                }
                
                // 注释
                alertJson["annotations"] = json::object();
                auto annotations = alert.getAnnotations();
                for (const auto& annotation : annotations) {
                    alertJson["annotations"][annotation.first] = annotation.second;
                }
                
                alertsArray.push_back(alertJson);
            }
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", alertsArray}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/events/{id} - 获取特定告警事件
    service->GET("/api/v2/alarm/events/{id}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string alertId = ctx->param("id");
            auto alert = alertEngine->getAlertById(alertId);
            
            if (!alert) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "alert not found"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_NOT_FOUND);
                return ctx->send(resp.dump(2));
            }

            json alertJson;
            alertJson["id"] = alert->getId();
            alertJson["fingerprint"] = alert->getFingerprint();
            
            // 状态转换
            std::string statusStr;
            switch (alert->getStatus()) {
                case yw::alertv2::AlertStatus::Pending:
                    statusStr = "pending";
                    break;
                case yw::alertv2::AlertStatus::Firing:
                    statusStr = "firing";
                    break;
                case yw::alertv2::AlertStatus::Resolved:
                    statusStr = "resolved";
                    break;
                default:
                    statusStr = "unknown";
                    break;
            }
            alertJson["status"] = statusStr;
            
            // 时间字段
            alertJson["created_at"] = alert->getCreatedAt();
            alertJson["updated_at"] = alert->getUpdatedAt();
            alertJson["starts_at"] = alert->getStartsAt();
            alertJson["ends_at"] = alert->getEndsAt();
            
            // 标签
            alertJson["labels"] = json::object();
            auto labels = alert->getLabels();
            for (const auto& label : labels) {
                alertJson["labels"][label.first] = label.second;
            }
            
            // 注释
            alertJson["annotations"] = json::object();
            auto annotations = alert->getAnnotations();
            for (const auto& annotation : annotations) {
                alertJson["annotations"][annotation.first] = annotation.second;
            }

            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", alertJson}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count - 获取告警总数
    service->GET("/api/v2/alarm/count", [alertEngine](const HttpContextPtr& ctx) {
        try {
            size_t count = alertEngine->getAlertCount();
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/status/{status} - 根据状态获取告警数量
    service->GET("/api/v2/alarm/count/status/{status}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string status = ctx->param("status");
            size_t count = alertEngine->getAlertCountByStatus(status);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"status", status}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/severity/{severity} - 根据严重程度获取告警数量
    service->GET("/api/v2/alarm/count/severity/{severity}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string severity = ctx->param("severity");
            size_t count = alertEngine->getAlertCountBySeverity(severity);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"severity", severity}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/type/{alert_type} - 根据告警类型获取告警数量
    service->GET("/api/v2/alarm/count/type/{alert_type}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string alertType = ctx->param("alert_type");
            size_t count = alertEngine->getAlertCountByAlertType(alertType);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"alert_type", alertType}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/host/{host_ip} - 根据主机IP获取告警数量
    service->GET("/api/v2/alarm/count/host/{host_ip}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string hostIp = ctx->param("host_ip");
            size_t count = alertEngine->getAlertCountByHostIp(hostIp);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"host_ip", hostIp}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/rule/{rule_id} - 根据规则ID获取告警数量
    service->GET("/api/v2/alarm/count/rule/{rule_id}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("rule_id");
            size_t count = alertEngine->getAlertCountByRuleId(ruleId);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"rule_id", ruleId}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/name/{alert_name} - 根据告警名称获取告警数量
    service->GET("/api/v2/alarm/count/name/{alert_name}", [alertEngine](const HttpContextPtr& ctx) {
        try {
            const std::string alertName = ctx->param("alert_name");
            size_t count = alertEngine->getAlertCountByAlertName(alertName);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"alert_name", alertName}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/time-range - 根据时间范围获取告警数量
    service->GET("/api/v2/alarm/count/time-range", [alertEngine](const HttpContextPtr& ctx) {
        try {
            auto params = ctx->params();
            
            // 获取开始时间和结束时间参数
            std::string startTime, endTime;
            if (params.find("start_time") != params.end()) {
                startTime = params["start_time"];
            }
            if (params.find("end_time") != params.end()) {
                endTime = params["end_time"];
            }
            
            if (startTime.empty() || endTime.empty()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "start_time and end_time parameters are required"}, 
                           {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
            
            size_t count = alertEngine->getAlertCountByTimeRange(startTime, endTime);
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"start_time", startTime}, {"end_time", endTime}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/active - 获取活跃告警数量
    service->GET("/api/v2/alarm/count/active", [alertEngine](const HttpContextPtr& ctx) {
        try {
            size_t count = alertEngine->getActiveAlertCount();
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"status", "firing"}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/pending - 获取等待告警数量
    service->GET("/api/v2/alarm/count/pending", [alertEngine](const HttpContextPtr& ctx) {
        try {
            size_t count = alertEngine->getPendingAlertCount();
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"status", "pending"}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/count/resolved - 获取已解决告警数量
    service->GET("/api/v2/alarm/count/resolved", [alertEngine](const HttpContextPtr& ctx) {
        try {
            size_t count = alertEngine->getResolvedAlertCount();
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", {{"status", "resolved"}, {"count", count}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /api/v2/alarm/statistics - 获取综合告警统计信息
    service->GET("/api/v2/alarm/statistics", [alertEngine](const HttpContextPtr& ctx) {
        try {
            // 获取各种状态的告警数量
            size_t totalCount = alertEngine->getAlertCount();
            size_t activeCount = alertEngine->getActiveAlertCount();
            size_t pendingCount = alertEngine->getPendingAlertCount();
            size_t resolvedCount = alertEngine->getResolvedAlertCount();
            
            // 按严重程度统计
            size_t criticalCount = alertEngine->getAlertCountBySeverity("严重");
            size_t warningCount = alertEngine->getAlertCountBySeverity("警告");
            size_t infoCount = alertEngine->getAlertCountBySeverity("信息");
            
            // 按告警类型统计
            size_t hardwareCount = alertEngine->getAlertCountByAlertType("硬件资源");
            size_t availabilityCount = alertEngine->getAlertCountByAlertType("availability");
            
            json statistics = json::object();
            
            // 总体统计
            statistics["total"] = totalCount;
            statistics["by_status"] = {
                {"firing", activeCount},
                {"pending", pendingCount},
                {"resolved", resolvedCount}
            };
            
            // 按严重程度统计
            statistics["by_severity"] = {
                {"严重", criticalCount},
                {"警告", warningCount},
                {"信息", infoCount}
            };
            
            // 按告警类型统计
            statistics["by_type"] = {
                {"硬件资源", hardwareCount},
                {"availability", availabilityCount}
            };
            
            json resp = {{"api_version", 2}, {"status", "success"}, 
                       {"data", statistics}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));

        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // POST /api/v2/alert/component - 组件状态告警上报
    service->POST("/api/v2/alert/component", [alertEngine](const HttpContextPtr& ctx) {
        try {
            auto body = ctx->body();
            if (body.empty()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "empty request body"}, {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }

            auto j = nlohmann::json::parse(body);
            
            // 提取组件信息
            std::string hostIp = j.value("host_ip", "");
            std::string instanceId = j.value("instance_id", "");
            std::string uuid = j.value("uuid", "");
            std::string index = j.value("index", "");
            std::string status = j.value("status", "unknown");
            
            // 验证必需字段
            if (hostIp.empty() || instanceId.empty()) {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "host_ip and instance_id are required"}, 
                           {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
            
            // 使用AlertEngine创建组件告警
            auto alert = alertEngine->createAlertFromComponent(hostIp, instanceId, uuid, index, status);
            
            if (alert) {
                json resp = {{"api_version", 2}, {"status", "success"}, 
                           {"data", {{"id", alert->getId()}, 
                                   {"fingerprint", alert->getFingerprint()},
                                   {"message", "Component alert created successfully"}}}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            } else {
                json resp = {{"api_version", 2}, {"status", "error"}, 
                           {"message", "failed to create component alert"}, 
                           {"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                return ctx->send(resp.dump(2));
            }

        } catch (const json::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "invalid JSON format: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            json resp = {{"api_version", 2}, {"status", "error"}, 
                       {"message", "internal error: " + std::string(e.what())}, 
                       {"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw
