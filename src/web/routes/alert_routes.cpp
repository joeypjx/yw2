// ============================================================================
// 文件功能描述：
// 告警路由（alert_routes）的实现文件，提供告警规则和告警事件相关的HTTP API端点。
// 主要功能包括：
// 1. 告警规则管理：POST/GET/PUT/DELETE /alert/rules - 告警规则的增删改查
// 2. 告警查询：GET /alert/alerts - 查询告警事件，支持按状态、严重程度、类型、IP、机箱号等过滤
// 3. 告警详情：GET /alert/alerts/{id} - 查询指定告警事件的详细信息
// 4. 告警统计：GET /alert/count - 获取告警总数
// 5. 参数解析：解析HTTP请求参数，构建告警过滤条件
// 6. 数据验证：验证告警规则和告警事件的格式和有效性
// ============================================================================

// 先包含third_party/json，确保使用正确的版本
#include <nlohmann/json.hpp>
#include <algorithm>

// 然后包含其他头文件
#include "alert_routes.h"
#include "utils/response_builder.h"
#include "alert/alert.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;
using ResponseBuilder = yw::utils::ResponseBuilder;

namespace {
    // 辅助函数：解析请求体为 JSON
    // ctx: HTTP上下文
    // 返回: 解析后的JSON对象，请求体为空时抛出异常
    json parseRequestBody(const HttpContextPtr& ctx) {
        auto body = ctx->body();
        if (body.empty()) {
            throw std::runtime_error("empty request body");
        }
        return json::parse(body);
    }

    // 辅助函数：从HTTP请求参数构建告警过滤条件
    // params: HTTP请求参数字典
    // 返回: 告警过滤条件对象（包含状态、严重程度、类型、IP、机箱号、槽位号等）
    template<typename MapType>
    yw::alert::AlertFilters buildAlertFilters(const MapType& params) {
        yw::alert::AlertFilters filters;
        
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
        
        // 验证并设置 limit（默认100，范围1-1000）
        int limit = ResponseBuilder::getIntParam(params, "limit", 100);
        filters.limit = std::max(1, std::min(1000, limit));
        
        return filters;
    }

    // 辅助函数：限制 JSON 数组大小
    // array: 要限制的JSON数组
    // maxSize: 最大元素数量
    // 返回: 限制后的JSON数组（如果原数组小于等于maxSize则返回原数组）
    json limitArraySize(const json& array, size_t maxSize) {
        if (!array.is_array() || array.size() <= maxSize) {
            return array;
        }
        json limited = json::array();
        for (size_t i = 0; i < maxSize; ++i) {
            limited.push_back(array[i]);
        }
        return limited;
    }
}

// 注册告警相关的HTTP路由
// service: HTTP服务实例
// alertModule: 告警模块实例
void registerAlertRoutes(hv::HttpService* service, 
                          std::shared_ptr<yw::alert::IAlertModule> alertModule) {
    if (!service || !alertModule) return;

    // POST /alarm/rules - 创建告警规则
    // 请求体：告警规则JSON对象
    service->POST("/alarm/rules", [alertModule](const HttpContextPtr& ctx) {
        try {
            auto ruleJson = parseRequestBody(ctx);
            
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
    // 返回：告警规则JSON数组
    service->GET("/alarm/rules", [alertModule](const HttpContextPtr& ctx) {
        try {
            json rulesArray = alertModule->getAllAlertRules();
            return ResponseBuilder::sendSuccessWithReturn(ctx, rulesArray);

        } catch (const std::exception& e) {
            return ResponseBuilder::sendErrorWithReturn(ctx, "internal error: " + std::string(e.what()));
        }
    });

    // GET /alarm/rules/{id} - 获取特定告警规则
    // 路径参数：id - 告警规则ID
    // 返回：告警规则JSON对象，不存在时返回404
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
    // 路径参数：id - 告警规则ID
    // 请求体：告警规则JSON对象
    service->POST("/alarm/rules/{id}/update", [alertModule](const HttpContextPtr& ctx) {
        try {
            const std::string ruleId = ctx->param("id");
            auto ruleJson = parseRequestBody(ctx);
            
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
    // 路径参数：id - 告警规则ID
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

    // GET /alarm/events - 获取告警事件列表
    // 查询参数：status, severity, alert_type, host_ip, box_id, slot_id, description, start_time, end_time, limit等
    // 返回：告警事件JSON数组
    service->GET("/alarm/events", [alertModule](const HttpContextPtr& ctx) {
        try {
            auto params = ctx->params();
            yw::alert::AlertFilters filters = buildAlertFilters(params);
            
            // 使用统一的过滤接口查询告警
            json alertsArray;
            if (!filters.hasAnyFilter()) {
                alertsArray = limitArraySize(alertModule->getAlertsExceptPending(), filters.limit);
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
    // 路径参数：id - 告警事件ID
    // 返回：告警事件JSON对象，不存在时返回404
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
    // 返回：告警总数JSON对象
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
