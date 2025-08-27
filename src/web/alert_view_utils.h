#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "yw/alert_model.h"

namespace yw {
namespace web {

// 时间/单位工具
std::string formatTimestampMs(std::int64_t ms);
long long   parseDurationSeconds(const std::string& s);
std::string formatSecondsCompact(long long sec);

// 表达式互转
nlohmann::json parseExpressionObject(const std::string& expr);
std::string    buildExpressionString(const nlohmann::json& exprObj);
std::string    extractMetricNameFromExpression(const std::string& expr);

// 规则转换
alert::UserAlertRule toUserAlertRule(const alert::Rule& r);
alert::Rule         fromUserAlertRule(const alert::UserAlertRule& ur);

// 事件转换
alert::UserAlertEventView toUserAlertEventView(const alert::AlertEvent& e);
std::vector<alert::UserAlertEventView> toUserAlertEventViews(const std::vector<alert::AlertEvent>& es);

} // namespace web
} // namespace yw


