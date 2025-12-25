#include "alert_query_service.h"
#include <spdlog/spdlog.h>

namespace yw {
namespace alert {

AlertQueryService::AlertQueryService(std::shared_ptr<AlertEventRepository> alertRepo)
    : alertRepo_(alertRepo) {
    if (!alertRepo_) {
        throw std::invalid_argument("AlertEventRepository不能为空");
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByStatus(const std::string& status) {
    try {
        return alertRepo_->getAlertsByStatus(status);
    } catch (const std::exception& e) {
        spdlog::error("根据状态获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByHostIp(const std::string& hostIp) {
    try {
        return alertRepo_->getAlertsByHostIp(hostIp);
    } catch (const std::exception& e) {
        spdlog::error("根据主机IP获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByBoxId(int boxId) {
    try {
        return alertRepo_->getAlertsByBoxId(boxId);
    } catch (const std::exception& e) {
        spdlog::error("根据机箱号获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsBySlotId(int slotId) {
    try {
        return alertRepo_->getAlertsBySlotId(slotId);
    } catch (const std::exception& e) {
        spdlog::error("根据板卡号获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) {
    try {
        return alertRepo_->getAlertsByTimeRange(startTime, endTime);
    } catch (const std::exception& e) {
        spdlog::error("根据时间范围获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByAlertType(const std::string& alertType) {
    try {
        return alertRepo_->getAlertsByAlertType(alertType);
    } catch (const std::exception& e) {
        spdlog::error("根据告警类型获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsBySeverity(const std::string& severity) {
    try {
        return alertRepo_->getAlertsBySeverity(severity);
    } catch (const std::exception& e) {
        spdlog::error("根据严重程度获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByDescription(const std::string& description) {
    try {
        return alertRepo_->getAlertsByDescription(description);
    } catch (const std::exception& e) {
        spdlog::error("根据描述获取告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsExceptPending() {
    try {
        return alertRepo_->getAlertsExceptPending();
    } catch (const std::exception& e) {
        spdlog::error("获取除Pending外的告警失败: {}", e.what());
        return {};
    }
}

std::vector<AlertEvent> AlertQueryService::getAlertsByFilters(const AlertFilters& filters) {
    try {
        return alertRepo_->getAlertsByFilters(filters);
    } catch (const std::exception& e) {
        spdlog::error("根据过滤条件获取告警失败: {}", e.what());
        return {};
    }
}

std::shared_ptr<AlertEvent> AlertQueryService::getAlertById(const std::string& alertId) {
    try {
        return alertRepo_->getAlertById(alertId);
    } catch (const std::exception& e) {
        spdlog::error("根据ID获取告警失败: {}", e.what());
        return nullptr;
    }
}

size_t AlertQueryService::getAlertCount() {
    try {
        return alertRepo_->getAlertCount();
    } catch (const std::exception& e) {
        spdlog::error("获取告警总数失败: {}", e.what());
        return 0;
    }
}

} // namespace alert
} // namespace yw

