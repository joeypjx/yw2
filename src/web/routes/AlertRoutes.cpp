// 先包含third_party/json，确保使用正确的版本
#include "../../../third_party/json/include/nlohmann/json.hpp"

// 然后包含其他头文件
#include "AlertRoutes.h"
#include "../../alert/application/AlertEngine.h"
#include "../../alert/domain/AlertRule.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerAlertRoutes(hv::HttpService* service, 
                          std::shared_ptr<yw::alert::AlertEngine> alertEngine) {
    if (!service || !alertEngine) return;

    // POST /alarm/rules - 创建告警规则
    service->POST("/alarm/rules", [alertEngine](const HttpContextPtr& ctx) {
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
            yw::alert::AlertRule rule = yw::alert::AlertRule::fromJson(ruleJson);
            
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

    // GET /alarm/rules - 获取所有告警规则
    service->GET("/alarm/rules", [alertEngine](const HttpContextPtr& ctx) {
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

    // GET /alarm/rules/{id} - 获取特定告警规则
    service->GET("/alarm/rules/{id}", [alertEngine](const HttpContextPtr& ctx) {
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

    // POST /alarm/rules/{id}/update - 更新告警规则
    service->POST("/alarm/rules/{id}/update", [alertEngine](const HttpContextPtr& ctx) {
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
            yw::alert::AlertRule rule = yw::alert::AlertRule::fromJson(ruleJson);
            
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

    // POST /alarm/rules/{id}/delete - 删除告警规则
    service->POST("/alarm/rules/{id}/delete", [alertEngine](const HttpContextPtr& ctx) {
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

    // GET /alarm/events - 获取告警事件
    service->GET("/alarm/events", [alertEngine](const HttpContextPtr& ctx) {
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
            
            // 获取机箱号过滤参数
            int boxIdFilter = -1;
            if (params.find("box_id") != params.end()) {
                try {
                    boxIdFilter = std::stoi(params["box_id"]);
                } catch (...) {
                    boxIdFilter = -1;
                }
            }
            
            // 获取板卡号过滤参数
            int slotIdFilter = -1;
            if (params.find("slot_id") != params.end()) {
                try {
                    slotIdFilter = std::stoi(params["slot_id"]);
                } catch (...) {
                    slotIdFilter = -1;
                }
            }
            
            // 获取描述过滤参数
            std::string descriptionFilter;
            if (params.find("description") != params.end()) {
                descriptionFilter = params["description"];
            }
            
            // 获取起止时间过滤参数
            std::string startTimeFilter;
            std::string endTimeFilter;
            if (params.find("start_time") != params.end()) {
                startTimeFilter = params["start_time"];
            }
            if (params.find("end_time") != params.end()) {
                endTimeFilter = params["end_time"];
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
            
            std::vector<yw::alert::Alert> alerts;
            
            // 根据过滤条件获取告警（优先级：时间范围 > 状态 > 严重程度 > 告警类型 > 主机IP > 描述 > 机箱号 > 板卡号）
            if (!startTimeFilter.empty() && !endTimeFilter.empty()) {
                // 时间范围查询（最高优先级）
                alerts = alertEngine->getAlertsByTimeRange(startTimeFilter, endTimeFilter);
            } else if (!statusFilter.empty()) {
                alerts = alertEngine->getAlertsByStatus(statusFilter);
            } else if (!severityFilter.empty()) {
                alerts = alertEngine->getAlertsBySeverity(severityFilter);
            } else if (!alertTypeFilter.empty()) {
                alerts = alertEngine->getAlertsByAlertType(alertTypeFilter);
            } else if (!hostIpFilter.empty()) {
                alerts = alertEngine->getAlertsByHostIp(hostIpFilter);
            } else if (!descriptionFilter.empty()) {
                alerts = alertEngine->getAlertsByDescription(descriptionFilter);
            } else if (boxIdFilter >= 0) {
                alerts = alertEngine->getAlertsByBoxId(boxIdFilter);
            } else if (slotIdFilter >= 0) {
                alerts = alertEngine->getAlertsBySlotId(slotIdFilter);
            } else {
                // 默认获取除Pending外的所有告警（Firing和Resolved）
                alerts = alertEngine->getAlertsExceptPending();
                
                // 如果结果太多，限制数量
                if (alerts.size() > static_cast<size_t>(limit)) {
                    alerts.resize(limit);
                }
            }
            
            // 如果使用了时间范围查询，还需要应用其他过滤条件
            if (!startTimeFilter.empty() && !endTimeFilter.empty()) {
                std::vector<yw::alert::Alert> filteredAlerts;
                for (const auto& alert : alerts) {
                    bool match = true;
                    
                    // 状态过滤
                    if (!statusFilter.empty()) {
                        std::string alertStatus;
                        switch (alert.getStatus()) {
                            case yw::alert::AlertStatus::Pending:
                                alertStatus = "pending";
                                break;
                            case yw::alert::AlertStatus::Firing:
                                alertStatus = "firing";
                                break;
                            case yw::alert::AlertStatus::Resolved:
                                alertStatus = "resolved";
                                break;
                            default:
                                alertStatus = "unknown";
                                break;
                        }
                        if (alertStatus != statusFilter) {
                            match = false;
                        }
                    }
                    
                    // 严重程度过滤
                    if (match && !severityFilter.empty()) {
                        auto labels = alert.getLabels();
                        auto it = labels.find("severity");
                        if (it == labels.end() || it->second != severityFilter) {
                            match = false;
                        }
                    }
                    
                    // 告警类型过滤
                    if (match && !alertTypeFilter.empty()) {
                        auto labels = alert.getLabels();
                        auto it = labels.find("alert_type");
                        if (it == labels.end() || it->second != alertTypeFilter) {
                            match = false;
                        }
                    }
                    
                    // 主机IP过滤
                    if (match && !hostIpFilter.empty()) {
                        auto labels = alert.getLabels();
                        auto it = labels.find("host_ip");
                        if (it == labels.end() || it->second != hostIpFilter) {
                            match = false;
                        }
                    }
                    
                    // 机箱号过滤
                    if (match && boxIdFilter >= 0) {
                        auto labels = alert.getLabels();
                        auto it = labels.find("box_id");
                        if (it == labels.end()) {
                            match = false;
                        } else {
                            try {
                                int boxId = std::stoi(it->second);
                                if (boxId != boxIdFilter) {
                                    match = false;
                                }
                            } catch (...) {
                                match = false;
                            }
                        }
                    }
                    
                    // 板卡号过滤
                    if (match && slotIdFilter >= 0) {
                        auto labels = alert.getLabels();
                        auto it = labels.find("slot_id");
                        if (it == labels.end()) {
                            match = false;
                        } else {
                            try {
                                int slotId = std::stoi(it->second);
                                if (slotId != slotIdFilter) {
                                    match = false;
                                }
                            } catch (...) {
                                match = false;
                            }
                        }
                    }
                    
                    // 描述过滤（模糊匹配）
                    if (match && !descriptionFilter.empty()) {
                        auto annotations = alert.getAnnotations();
                        auto it = annotations.find("description");
                        if (it == annotations.end()) {
                            match = false;
                        } else {
                            // 检查 description 是否包含过滤文本（不区分大小写）
                            std::string description = it->second;
                            std::string filterLower = descriptionFilter;
                            std::transform(description.begin(), description.end(), description.begin(), ::tolower);
                            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
                            if (description.find(filterLower) == std::string::npos) {
                                match = false;
                            }
                        }
                    }
                    
                    if (match) {
                        filteredAlerts.push_back(alert);
                    }
                }
                alerts = std::move(filteredAlerts);
            }
            
            // 如果结果太多，限制数量
            if (alerts.size() > static_cast<size_t>(limit)) {
                alerts.resize(limit);
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
                    case yw::alert::AlertStatus::Pending:
                        statusStr = "pending";
                        break;
                    case yw::alert::AlertStatus::Firing:
                        statusStr = "firing";
                        break;
                    case yw::alert::AlertStatus::Resolved:
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

    // GET /alarm/events/{id} - 获取特定告警事件
    service->GET("/alarm/events/{id}", [alertEngine](const HttpContextPtr& ctx) {
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
                case yw::alert::AlertStatus::Pending:
                    statusStr = "pending";
                    break;
                case yw::alert::AlertStatus::Firing:
                    statusStr = "firing";
                    break;
                case yw::alert::AlertStatus::Resolved:
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

    // GET /alarm/count - 获取告警总数
    service->GET("/alarm/count", [alertEngine](const HttpContextPtr& ctx) {
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

    // POST /alert/component - 组件状态告警上报
    service->POST("/alert/component", [alertEngine](const HttpContextPtr& ctx) {
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
            int index = j.value("index", 0);
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
