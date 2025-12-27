// ============================================================================
// 文件功能描述：
// 告警查询服务（AlertQueryService）的头文件，定义告警事件查询功能的接口。
// 主要功能包括：
// 1. 状态查询：根据告警状态（pending/firing/resolved）查询告警列表
// 2. 主机查询：根据节点IP地址查询告警列表
// 3. 机箱查询：根据机箱号查询告警列表
// 4. 过滤查询：根据多个过滤条件（状态、严重程度、类型、IP、机箱号、槽位号等）查询告警
// 5. 详情查询：根据告警ID查询单个告警事件的详细信息
// 6. 统计查询：获取告警总数（排除pending状态的告警）
// 7. 结果限制：所有查询方法都限制返回最多5000条记录，避免内存溢出
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
    
    // 根据状态获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsByStatus(const std::string& status);
    // 根据节点IP获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsByHostIp(const std::string& hostIp);
    // 根据机箱号获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsByBoxId(int boxId);
    // 根据槽位号获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsBySlotId(int slotId);
    // 根据时间范围获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime);
    // 根据告警类型获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsByAlertType(const std::string& alertType);
    // 根据严重程度获取告警事件列表（限制5000条）
    std::vector<AlertEvent> getAlertsBySeverity(const std::string& severity);
    // 根据描述内容获取告警事件列表（模糊匹配，限制5000条）
    std::vector<AlertEvent> getAlertsByDescription(const std::string& description);
    // 获取除Pending状态外的所有告警事件（限制5000条）
    std::vector<AlertEvent> getAlertsExceptPending();
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

