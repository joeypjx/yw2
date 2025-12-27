#include "alert_query_service.h"
#include <spdlog/spdlog.h>

namespace yw {
namespace alert {

// 告警查询服务构造函数
// alertRepo: 告警事件仓库实例，不能为空
AlertQueryService::AlertQueryService(std::shared_ptr<AlertEventRepository> alertRepo)
    : alertRepo_(alertRepo) {
    if (!alertRepo_) {
        throw std::invalid_argument("AlertEventRepository不能为空");
    }
}

// 根据状态获取告警列表
// status: 告警状态（pending/firing/resolved）
// 返回: 匹配状态的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByStatus(const std::string& status) {
    try {
        return alertRepo_->getAlertsByStatus(status);
    } catch (const std::exception& e) {
        spdlog::error("根据状态获取告警失败: {}", e.what());
        return {};
    }
}

// 根据主机IP获取告警列表
// hostIp: 节点IP地址
// 返回: 匹配IP地址的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByHostIp(const std::string& hostIp) {
    try {
        return alertRepo_->getAlertsByHostIp(hostIp);
    } catch (const std::exception& e) {
        spdlog::error("根据主机IP获取告警失败: {}", e.what());
        return {};
    }
}

// 根据机箱号获取告警列表
// boxId: 机箱编号（1-9）
// 返回: 匹配机箱号的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByBoxId(int boxId) {
    try {
        return alertRepo_->getAlertsByBoxId(boxId);
    } catch (const std::exception& e) {
        spdlog::error("根据机箱号获取告警失败: {}", e.what());
        return {};
    }
}

// 根据板卡槽位号获取告警列表
// slotId: 槽位编号（1-14）
// 返回: 匹配槽位号的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsBySlotId(int slotId) {
    try {
        return alertRepo_->getAlertsBySlotId(slotId);
    } catch (const std::exception& e) {
        spdlog::error("根据板卡号获取告警失败: {}", e.what());
        return {};
    }
}

// 根据时间范围获取告警列表
// startTime: 开始时间（PostgreSQL timestamp格式）
// endTime: 结束时间（PostgreSQL timestamp格式）
// 返回: 在指定时间范围内创建的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) {
    try {
        return alertRepo_->getAlertsByTimeRange(startTime, endTime);
    } catch (const std::exception& e) {
        spdlog::error("根据时间范围获取告警失败: {}", e.what());
        return {};
    }
}

// 根据告警类型获取告警列表
// alertType: 告警类型（如"硬件状态"、"业务链路"、"系统告警"等）
// 返回: 匹配类型的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByAlertType(const std::string& alertType) {
    try {
        return alertRepo_->getAlertsByAlertType(alertType);
    } catch (const std::exception& e) {
        spdlog::error("根据告警类型获取告警失败: {}", e.what());
        return {};
    }
}

// 根据严重程度获取告警列表
// severity: 严重程度（如"critical"、"warning"、"info"等）
// 返回: 匹配严重程度的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsBySeverity(const std::string& severity) {
    try {
        return alertRepo_->getAlertsBySeverity(severity);
    } catch (const std::exception& e) {
        spdlog::error("根据严重程度获取告警失败: {}", e.what());
        return {};
    }
}

// 根据描述内容获取告警列表（模糊匹配）
// description: 描述关键词，支持LIKE模糊匹配
// 返回: 描述中包含关键词的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByDescription(const std::string& description) {
    try {
        return alertRepo_->getAlertsByDescription(description);
    } catch (const std::exception& e) {
        spdlog::error("根据描述获取告警失败: {}", e.what());
        return {};
    }
}

// 获取除Pending状态外的所有告警（即Firing和Resolved状态）
// 返回: Firing和Resolved状态的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsExceptPending() {
    try {
        return alertRepo_->getAlertsExceptPending();
    } catch (const std::exception& e) {
        spdlog::error("获取除Pending外的告警失败: {}", e.what());
        return {};
    }
}

// 根据多个过滤条件组合查询告警
// filters: 过滤条件对象，包含状态、类型、时间范围等多个条件
// 返回: 匹配所有过滤条件的告警列表，失败时返回空列表
std::vector<AlertEvent> AlertQueryService::getAlertsByFilters(const AlertFilters& filters) {
    try {
        return alertRepo_->getAlertsByFilters(filters);
    } catch (const std::exception& e) {
        spdlog::error("根据过滤条件获取告警失败: {}", e.what());
        return {};
    }
}

// 根据告警ID获取单个告警
// alertId: 告警的唯一标识符
// 返回: 告警对象指针，不存在或失败时返回nullptr
std::shared_ptr<AlertEvent> AlertQueryService::getAlertById(const std::string& alertId) {
    try {
        return alertRepo_->getAlertById(alertId);
    } catch (const std::exception& e) {
        spdlog::error("根据ID获取告警失败: {}", e.what());
        return nullptr;
    }
}

// 获取告警总数
// 返回: 数据库中告警的总数量，失败时返回0
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

