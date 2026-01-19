// ============================================================================
// 文件功能描述：
// 告警查询服务（AlertQueryService）的实现文件，提供告警事件的查询功能。
// 主要功能包括：
// 1. 过滤查询：根据多个过滤条件（状态、严重程度、类型、IP、机箱号、槽位号等）查询告警
// 2. 详情查询：根据告警ID查询单个告警事件的详细信息
// 3. 统计查询：获取告警总数（排除pending状态的告警）
// 4. 错误处理：捕获和记录查询过程中的异常，返回空列表或nullptr
// ============================================================================

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

