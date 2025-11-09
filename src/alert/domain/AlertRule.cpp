#include "AlertRule.h"
#include "AlertRuleEvaluator.h"
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <random>
#include <ctime>

namespace yw {
namespace alert {

AlertRule::AlertRule(const std::string& name, const AlertExpression& expr, 
                     const std::string& duration, const std::string& severity,
                     const std::string& summary, const std::string& description,
                     const std::string& alertType)
    : alert_name_(name), expression_(expr), for_(duration), 
      severity_(severity), summary_(summary), description_(description), 
      alert_type_(alertType), enabled_(true) {
    // 自动生成系统字段
    generateId();
    setCreatedNow();
    setUpdatedNow();
}

nlohmann::json AlertRule::toJson() const {
    nlohmann::json j;
    to_json(j, *this);
    return j;
}

AlertRule AlertRule::fromJson(const nlohmann::json& j) {
    AlertRule rule;
    
    // 解析id（如果存在）
    if (j.contains("id")) {
        j.at("id").get_to(rule.id_);
    } else {
        // 如果没有id，生成一个新的
        rule.generateId();
    }
    
    // 解析alert_name
    if (j.contains("alert_name")) {
        j.at("alert_name").get_to(rule.alert_name_);
    }
    
    // 解析expression
    if (j.contains("expression")) {
        j.at("expression").get_to(rule.expression_);
    }
    
    // 解析for
    if (j.contains("for")) {
        j.at("for").get_to(rule.for_);
    }
    
    // 解析severity
    if (j.contains("severity")) {
        j.at("severity").get_to(rule.severity_);
    }
    
    // 解析summary
    if (j.contains("summary")) {
        j.at("summary").get_to(rule.summary_);
    }
    
    // 解析description
    if (j.contains("description")) {
        j.at("description").get_to(rule.description_);
    }
    
    // 解析alert_type
    if (j.contains("alert_type")) {
        j.at("alert_type").get_to(rule.alert_type_);
    }
    
    // 解析created_at
    if (j.contains("created_at")) {
        j.at("created_at").get_to(rule.created_at_);
    } else {
        // 如果没有创建时间，设置为当前时间
        rule.setCreatedNow();
    }
    
    // 解析updated_at
    if (j.contains("updated_at")) {
        j.at("updated_at").get_to(rule.updated_at_);
    } else {
        // 如果没有更新时间，设置为当前时间
        rule.setUpdatedNow();
    }
    
    // 解析enabled
    if (j.contains("enabled")) {
        j.at("enabled").get_to(rule.enabled_);
    } else {
        // 如果没有enabled字段，默认为true
        rule.enabled_ = true;
    }
    
    return rule;
}

bool AlertRule::isValid() const {
    // 检查id是否为空
    if (id_.empty()) {
        return false;
    }
    
    // 检查alert_name是否为空
    if (alert_name_.empty()) {
        return false;
    }
    
    // 检查expression是否有效
    if (expression_.stable.empty() || expression_.metric.empty()) {
        return false;
    }
    
    // 检查stable是否为有效的资源类型
    const std::vector<std::string> validStables = {
        "cpu", "memory", "disk", "network", "gpu", "alive"
    };
    bool validStable = false;
    for (const auto& stable : validStables) {
        if (expression_.stable == stable) {
            validStable = true;
            break;
        }
    }
    if (!validStable) {
        return false;
    }
    
    // 检查conditions是否为空
    if (expression_.conditions.empty()) {
        return false;
    }
    
    // 检查每个condition的operator是否有效
    const std::vector<std::string> validOperators = {
        ">", "<", ">=", "<=", "==", "!="
    };
    for (const auto& condition : expression_.conditions) {
        bool validOp = false;
        for (const auto& op : validOperators) {
            if (condition.operator_ == op) {
                validOp = true;
                break;
            }
        }
        if (!validOp) {
            return false;
        }
    }
    
    // 检查for格式是否有效（简单检查是否包含s/m/h）
    if (!for_.empty()) {
        bool hasTimeUnit = false;
        if (for_.find('s') != std::string::npos ||
            for_.find('m') != std::string::npos ||
            for_.find('h') != std::string::npos) {
            hasTimeUnit = true;
        }
        if (!hasTimeUnit) {
            return false;
        }
    }
    
    // 检查severity是否为空
    if (severity_.empty()) {
        return false;
    }
    
    // 检查summary是否为空
    if (summary_.empty()) {
        return false;
    }
    
    // 检查alert_type是否为空
    if (alert_type_.empty()) {
        return false;
    }
    
    // 检查created_at是否为空
    if (created_at_.empty()) {
        return false;
    }
    
    // 检查updated_at是否为空
    if (updated_at_.empty()) {
        return false;
    }
    
    return true;
}

std::string AlertRule::getValidationError() const {
    if (id_.empty()) {
        return "id不能为空";
    }
    
    if (alert_name_.empty()) {
        return "alert_name不能为空";
    }
    
    if (expression_.stable.empty()) {
        return "expression.stable不能为空";
    }
    
    if (expression_.metric.empty()) {
        return "expression.metric不能为空";
    }
    
    // 检查stable是否为有效的资源类型
    const std::vector<std::string> validStables = {
        "cpu", "memory", "disk", "network", "gpu", "alive"
    };
    bool validStable = false;
    for (const auto& stable : validStables) {
        if (expression_.stable == stable) {
            validStable = true;
            break;
        }
    }
    if (!validStable) {
        return "expression.stable必须是cpu, memory, disk, network, gpu, alive之一";
    }
    
    if (expression_.conditions.empty()) {
        return "expression.conditions不能为空";
    }
    
    // 检查每个condition的operator是否有效
    const std::vector<std::string> validOperators = {
        ">", "<", ">=", "<=", "==", "!="
    };
    for (const auto& condition : expression_.conditions) {
        bool validOp = false;
        for (const auto& op : validOperators) {
            if (condition.operator_ == op) {
                validOp = true;
                break;
            }
        }
        if (!validOp) {
            return "condition.operator必须是>, <, >=, <=, ==, !=之一";
        }
    }
    
    if (!for_.empty()) {
        bool hasTimeUnit = false;
        if (for_.find('s') != std::string::npos ||
            for_.find('m') != std::string::npos ||
            for_.find('h') != std::string::npos) {
            hasTimeUnit = true;
        }
        if (!hasTimeUnit) {
            return "for必须包含时间单位(s/m/h)";
        }
    }
    
    if (severity_.empty()) {
        return "severity不能为空";
    }
    
    if (summary_.empty()) {
        return "summary不能为空";
    }
    
    if (alert_type_.empty()) {
        return "alert_type不能为空";
    }
    
    if (created_at_.empty()) {
        return "created_at不能为空";
    }
    
    if (updated_at_.empty()) {
        return "updated_at不能为空";
    }
    
    return "";
}

void AlertRule::generateId() {
    // 使用时间戳和随机数生成唯一ID
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::ostringstream oss;
    oss << "rule_" << std::put_time(::localtime(&time_t), "%Y%m%d_%H%M%S")
        << "_" << ms.count() << "_" << dis(gen);
    
    id_ = oss.str();
}

void AlertRule::setCreatedNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    created_at_ = oss.str();
}

void AlertRule::setUpdatedNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    updated_at_ = oss.str();
}

} // namespace alert
} // namespace yw
