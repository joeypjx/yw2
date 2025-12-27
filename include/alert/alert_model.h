// ============================================================================
// 文件功能描述：
// 告警模型（alert_model）的头文件，定义告警系统相关的数据模型和结构。
// 主要功能包括：
// 1. 告警状态枚举：定义AlertStatus枚举（Pending、Firing、Resolved）
// 2. 告警表达式模型：定义AlertExpression结构体，表示告警规则的查询表达式
// 3. 告警条件模型：定义AlertCondition结构体，表示告警规则的条件（操作符、阈值等）
// 4. 告警过滤模型：定义AlertFilters结构体，用于告警查询的过滤条件
// 5. JSON序列化：提供JSON序列化和反序列化支持，便于数据持久化和API交互
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace yw {
namespace alert {

//=============================================================================
// 告警状态
//=============================================================================

/**
 * @brief 告警状态枚举
 */
enum class AlertStatus {
    Pending,    ///< 匹配但还未满足持续时间条件
    Firing,     ///< 触发告警
    Resolved    ///< 告警已解决
};

NLOHMANN_JSON_SERIALIZE_ENUM(AlertStatus, {
    {AlertStatus::Pending, "pending"},
    {AlertStatus::Firing, "firing"},
    {AlertStatus::Resolved, "resolved"}
})

//=============================================================================
// 告警条件与表达式
//=============================================================================

/**
 * @brief 告警条件
 */
struct AlertCondition {
    std::string operator_;    ///< 操作符: >, <, >=, <=, ==, !=
    double threshold;         ///< 阈值
    
    AlertCondition() = default;
    AlertCondition(const std::string& op, double thresh) 
        : operator_(op), threshold(thresh) {}
};

/**
 * @brief 告警表达式
 */
struct AlertExpression {
    std::string stable;                           ///< 资源类型: cpu, memory, disk, network, gpu, alive
    std::string metric;                           ///< 指标名称
    std::vector<AlertCondition> conditions;       ///< 条件列表（所有条件需同时满足）
    std::vector<std::unordered_map<std::string, std::string>> tags;  ///< 标签匹配列表
    
    AlertExpression() = default;
};

//=============================================================================
// 告警查询过滤条件
//=============================================================================

/**
 * @brief 告警查询过滤条件
 */
struct AlertFilters {
    std::string status;              ///< 状态过滤 (pending/firing/resolved)
    std::string severity;            ///< 严重程度过滤
    std::string alert_type;          ///< 告警类型过滤
    std::string host_ip;             ///< 主机IP过滤
    int box_id = -1;                 ///< 机箱号过滤 (-1 表示不过滤)
    int slot_id = -1;                ///< 板卡号过滤 (-1 表示不过滤)
    std::string start_time;          ///< 起始时间过滤
    std::string end_time;            ///< 结束时间过滤
    std::string description;         ///< 描述过滤（模糊匹配）
    int limit = 100;                 ///< 限制返回数量 (默认100, 最大1000)
    std::string stack_name;          ///< 栈名过滤
    std::string component_name;      ///< 组件名过滤
    
    /**
     * @brief 检查是否有任何过滤条件
     */
    bool hasAnyFilter() const {
        return !status.empty() || !severity.empty() || !alert_type.empty() ||
               !host_ip.empty() || box_id >= 0 || slot_id >= 0 ||
               !start_time.empty() || !end_time.empty() || !description.empty() ||
               !stack_name.empty() || !component_name.empty();
    }
};

//=============================================================================
// JSON 序列化支持
//=============================================================================

inline void to_json(nlohmann::json& j, const AlertCondition& c) {
    j = nlohmann::json{
        {"operator", c.operator_},
        {"threshold", c.threshold}
    };
}

inline void from_json(const nlohmann::json& j, AlertCondition& c) {
    j.at("operator").get_to(c.operator_);
    j.at("threshold").get_to(c.threshold);
}

inline void to_json(nlohmann::json& j, const AlertExpression& e) {
    j = nlohmann::json{
        {"stable", e.stable},
        {"metric", e.metric},
        {"conditions", e.conditions},
        {"tags", e.tags}
    };
}

inline void from_json(const nlohmann::json& j, AlertExpression& e) {
    j.at("stable").get_to(e.stable);
    j.at("metric").get_to(e.metric);
    j.at("conditions").get_to(e.conditions);
    if (j.contains("tags")) {
        j.at("tags").get_to(e.tags);
    }
}

} // namespace alert
} // namespace yw

