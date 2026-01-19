// ============================================================================
// 文件功能描述：
// 告警查询服务（AlertQueryService）的头文件，定义告警事件查询功能的接口。
// 主要功能包括：
// 1. 过滤查询：根据多个过滤条件（状态、严重程度、类型、IP、机箱号、槽位号等）查询告警
// 2. 详情查询：根据告警ID查询单个告警事件的详细信息
// 3. 统计查询：获取告警总数（排除pending状态的告警）
// ============================================================================

#pragma once

#include "../domain/alert_event.h"
#include "../infrastructure/alert_event_repository.h"
#include "alert/alert_model.h"  // AlertFilters
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

/**
 * @brief 告警查询服务类
 * 
 * 负责提供各种告警查询功能
 */
class AlertQueryService {
public:
    explicit AlertQueryService(std::shared_ptr<AlertEventRepository> alertRepo);
    ~AlertQueryService() = default;
    
    // 根据多个过滤条件组合查询告警事件
    std::vector<AlertEvent> getAlertsByFilters(const AlertFilters& filters);
    // 根据ID获取告警事件
    std::shared_ptr<AlertEvent> getAlertById(const std::string& alertId);
    // 获取告警事件总数
    size_t getAlertCount();

private:
    std::shared_ptr<AlertEventRepository> alertRepo_;
};

} // namespace alert
} // namespace yw

