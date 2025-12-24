// 先包含third_party/json，确保使用正确的版本
#include <nlohmann/json.hpp>

// 然后包含其他头文件
#include "alert_routes.h"
#include "utils/response_builder.h"
#include "alert/alert.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;
using ResponseBuilder = yw::utils::ResponseBuilder;

void registerAlertRoutes(hv::HttpService* service, 
                          std::shared_ptr<yw::alert::IAlertModule> alertModule) {
    if (!service || !alertModule) return;

    // POST /alarm/rules - 创建告警规则
    service->POST("/alarm/rules", [alertModule](const HttpContextPtr& ctx) {
        try {
            auto body = ctx->body();
            if (body.empty()) {
                return ResponseBuilder::sendErrorWithReturn(ctx, "empty request body", HTTP_STATUS_BAD_REQUEST);
            }

            auto ruleJson = json::parse(body);
            
            // 使用 IAlertModule 接口保存规则
            bool success = alertModule->addAlertRule(ruleJson);
            if (success) {
                std::string ruleId = ruleJson.value("id", "");
                json data = {{"id", ruleId}, {"message", "Rule created successfully"}};
                return ResponseBuilder::sendSuccessWithReturn(ctx, data);
            } else {
                return ResponseBuilder::sendErrorWithReturn(ctx, "failed to create rule");
            }

        } catch (const json::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "invalid JSON format: " + std::string(e.what()), 
                                                       HTTP_STATUS_BAD_REQUEST);
        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "internal error: " + std::string(e.what()));
        }
    });

    // GET /alarm/rules - 获取所有告警规则
    service->GET("/alarm/rules", [alertModule](const HttpContextPtr& ctx) {
        try {
            json rulesArray = alertModule->getAllAlertRules();
            return ResponseBuilder::sendSuccessWithReturn(ctx, rulesArray);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "internal error: " + std::string(e.what()));
        }
    });

    // GET /alarm/rules/{id} - 获取特定告警规则
    service->GET("/alarm/rules/{id}", [alertModule](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            json rule = alertModule->getAlertRuleById(ruleId);
            
            if (rule.empty()) {
                return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                           "rule not found", 
                                                           HTTP_STATUS_NOT_FOUND);
            }

            return ResponseBuilder::sendSuccessWithReturn(ctx, rule);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "internal error: " + std::string(e.what()));
        }
    });

    // POST /alarm/rules/{id}/update - 更新告警规则
    service->POST("/alarm/rules/{id}/update", [alertModule](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            auto body = ctx->body();
            
            if (body.empty()) {
                return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                           "empty request body", 
                                                           HTTP_STATUS_BAD_REQUEST);
            }

            auto ruleJson = json::parse(body);
            
            // 确保ID匹配
            ruleJson["id"] = ruleId;

            // 使用 IAlertModule 接口更新规则
            bool success = alertModule->updateAlertRule(ruleJson);
            if (success) {
                json data = {{"id", ruleId}, {"message", "Rule updated successfully"}};
                return ResponseBuilder::sendSuccessWithReturn(ctx, data);
            } else {
                return ResponseBuilder::sendErrorWithReturn(ctx, "failed to update rule");
            }

        } catch (const json::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "invalid JSON format: " + std::string(e.what()), 
                                                       HTTP_STATUS_BAD_REQUEST);
        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "internal error: " + std::string(e.what()));
        }
    });

    // POST /alarm/rules/{id}/delete - 删除告警规则
    service->POST("/alarm/rules/{id}/delete", [alertModule](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            bool success = alertModule->deleteAlertRule(ruleId);
            
            if (success) {
                json data = {{"id", ruleId}, {"message", "Rule deleted successfully"}};
                return ResponseBuilder::sendSuccessWithReturn(ctx, data);
            } else {
                return ResponseBuilder::sendErrorWithReturn(ctx, "failed to delete rule");
            }

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "internal error: " + std::string(e.what()));
        }
    });

    // GET /alarm/events - 获取告警事件
    service->GET("/alarm/events", [alertModule](const HttpContextPtr& ctx) {
        try {
            // 获取查询参数
            auto params = ctx->params();
            
            // 构建过滤条件
            yw::alert::AlertFilters filters;
            
            // 使用工具函数解析参数
            if (ResponseBuilder::hasParam(params, "status")) {
                filters.status = ResponseBuilder::getParam(params, "status");
            }
            if (ResponseBuilder::hasParam(params, "severity")) {
                filters.severity = ResponseBuilder::getParam(params, "severity");
            }
            if (ResponseBuilder::hasParam(params, "alert_type")) {
                filters.alert_type = ResponseBuilder::getParam(params, "alert_type");
            }
            if (ResponseBuilder::hasParam(params, "host_ip")) {
                filters.host_ip = ResponseBuilder::getParam(params, "host_ip");
            }
            if (ResponseBuilder::hasParam(params, "box_id")) {
                filters.box_id = ResponseBuilder::getIntParam(params, "box_id", -1);
            }
            if (ResponseBuilder::hasParam(params, "slot_id")) {
                filters.slot_id = ResponseBuilder::getIntParam(params, "slot_id", -1);
            }
            if (ResponseBuilder::hasParam(params, "description")) {
                filters.description = ResponseBuilder::getParam(params, "description");
            }
            if (ResponseBuilder::hasParam(params, "start_time")) {
                filters.start_time = ResponseBuilder::getParam(params, "start_time");
            }
            if (ResponseBuilder::hasParam(params, "end_time")) {
                filters.end_time = ResponseBuilder::getParam(params, "end_time");
            }
            if (ResponseBuilder::hasParam(params, "stack_name")) {
                filters.stack_name = ResponseBuilder::getParam(params, "stack_name");
            }
            if (ResponseBuilder::hasParam(params, "component_name")) {
                filters.component_name = ResponseBuilder::getParam(params, "component_name");
            }
            
            // 获取限制数量参数（带验证）
            int limit = ResponseBuilder::getIntParam(params, "limit", 100);
            if (limit <= 0) limit = 100;
            if (limit > 1000) limit = 1000; // 最大限制1000条
            filters.limit = limit;
            
            // 使用统一的过滤接口查询告警
            json alertsArray;
            if (!filters.hasAnyFilter()) {
                alertsArray = alertModule->getAlertsExceptPending();
                // 限制数量
                if (alertsArray.is_array() && alertsArray.size() > static_cast<size_t>(filters.limit)) {
                    json limitedArray = json::array();
                    for (size_t i = 0; i < static_cast<size_t>(filters.limit); ++i) {
                        limitedArray.push_back(alertsArray[i]);
                    }
                    alertsArray = limitedArray;
                }
            } else {
                alertsArray = alertModule->getAlertsByFilters(filters);
            }
            
            return ResponseBuilder::sendSuccessWithReturn(ctx, alertsArray);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "internal error: " + std::string(e.what()));
        }
    });

    // GET /alarm/events/{id} - 获取特定告警事件
    service->GET("/alarm/events/{id}", [alertModule](const HttpContextPtr& ctx) {
        try {
            const std::string alertId = ctx->param("id");
            json alertJson = alertModule->getAlertById(alertId);
            
            if (alertJson.empty()) {
                return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                           "alert not found", 
                                                           HTTP_STATUS_NOT_FOUND);
            }

            return ResponseBuilder::sendSuccessWithReturn(ctx, alertJson);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "internal error: " + std::string(e.what()));
        }
    });

    // GET /alarm/count - 获取告警总数
    service->GET("/alarm/count", [alertModule](const HttpContextPtr& ctx) {
        try {
            size_t count = alertModule->getAlertCount();
            json data = {{"count", count}};
            return ResponseBuilder::sendSuccessWithReturn(ctx, data);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, 
                                                       "internal error: " + std::string(e.what()));
        }
    });
}

} // namespace routes
} // namespace web
} // namespace yw
