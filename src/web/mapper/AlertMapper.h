#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "yw/alert_model.h"

namespace yw {
namespace web {
namespace mapper {

// 工具函数（API 层可用）
std::string   formatTimestampMs(std::int64_t ms);
long long     parseDurationSeconds(const std::string& s);
std::string   formatSecondsCompact(long long sec);

// 表达式互转（如需单独使用）
nlohmann::json parseExpressionObject(const std::string& expr);
std::string    buildExpressionString(const nlohmann::json& exprObj);
std::string    extractMetricNameFromExpression(const std::string& expr);

// 规则转换：内部 Rule ↔ 对外 UserAlertRule
web::UserAlertRule toUserAlertRule(const alert::Rule& r);
alert::Rule        fromUserAlertRule(const web::UserAlertRule& ur);

// 事件转换：内部 AlertEvent ↦ 对外 UserAlertEventView
web::UserAlertEventView toUserAlertEventView(const alert::AlertEvent& e);
std::vector<web::UserAlertEventView> toUserAlertEventViews(const std::vector<alert::AlertEvent>& es);

} // namespace mapper
} // namespace web
} // namespace yw




