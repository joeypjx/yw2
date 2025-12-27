// ============================================================================
// 文件功能描述：
// 告警事件DTO（alert_event_dto）的头文件，定义告警事件API响应的数据传输对象。
// 主要功能包括：
// 1. 告警事件视图定义：定义UserAlertEventView结构体，用于告警事件API的响应数据
// 2. 告警注释定义：定义AlertEventAnnotations结构体，表示告警的描述和摘要
// 3. JSON序列化：提供JSON序列化和反序列化支持，符合Prometheus Alertmanager格式
// 4. 数据转换：将内部告警事件模型转换为前端友好的格式
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace yw {
namespace web {

// 告警事件注释信息，用于存储告警的描述和摘要
struct AlertEventAnnotations {
    std::string description;  // 详细描述
    std::string summary;      // 摘要
};

// 告警事件视图，用于前端展示告警事件信息（符合Prometheus Alertmanager格式）
struct UserAlertEventView {
    AlertEventAnnotations annotations;  // 注释信息（描述和摘要）
    std::string           created_at;   // 创建时间（格式：YYYY-MM-DD HH:MM:SS）
    std::string           ends_at;      // 结束时间（格式：YYYY-MM-DD HH:MM:SS，未结束时为空）
    std::string           fingerprint;  // 告警指纹（唯一标识符）
    std::string           id;           // 告警ID（暂时使用fingerprint作为id）
    std::map<std::string, std::string> labels;  // 标签键值对（包含host_ip、severity等）
    std::string           starts_at;    // 开始时间（格式：YYYY-MM-DD HH:MM:SS）
    std::string           status;       // 状态（inactive/pending/firing/resolved）
    std::string           updated_at;   // 更新时间（格式：YYYY-MM-DD HH:MM:SS）
};

// JSON序列化宏，用于告警注释信息的自动序列化/反序列化
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEventAnnotations,
    description, summary)

// JSON序列化宏，用于告警事件视图的自动序列化/反序列化
// 支持与前端的JSON数据交互
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserAlertEventView,
    annotations, created_at, ends_at, fingerprint, id, labels, starts_at, status, updated_at)

} // namespace web
} // namespace yw

