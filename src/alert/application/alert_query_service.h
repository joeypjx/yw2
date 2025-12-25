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
    
    std::vector<AlertEvent> getAlertsByStatus(const std::string& status);
    std::vector<AlertEvent> getAlertsByHostIp(const std::string& hostIp);
    std::vector<AlertEvent> getAlertsByBoxId(int boxId);
    std::vector<AlertEvent> getAlertsBySlotId(int slotId);
    std::vector<AlertEvent> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime);
    std::vector<AlertEvent> getAlertsByAlertType(const std::string& alertType);
    std::vector<AlertEvent> getAlertsBySeverity(const std::string& severity);
    std::vector<AlertEvent> getAlertsByDescription(const std::string& description);
    std::vector<AlertEvent> getAlertsExceptPending();
    std::vector<AlertEvent> getAlertsByFilters(const AlertFilters& filters);
    std::shared_ptr<AlertEvent> getAlertById(const std::string& alertId);
    size_t getAlertCount();

private:
    std::shared_ptr<AlertEventRepository> alertRepo_;
};

} // namespace alert
} // namespace yw

