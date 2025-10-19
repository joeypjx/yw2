#pragma once

#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include "alert_services.h"

namespace yw {
namespace alert {

class DatabaseEventRepository : public IEventRepository {
public:
    explicit DatabaseEventRepository(std::shared_ptr<pqxx::connection> conn);
    
    bool append(const AlertEvent& event) override;
    std::vector<AlertEvent> query(const std::string& duration) const override;
    std::size_t countByStatus(AlertStatus status) const override;
    
    // 单事件模式：检查事件是否存在
    bool hasEvent(const std::string& fingerprint) const override;
    // 单事件模式：获取事件
    std::optional<AlertEvent> getEvent(const std::string& fingerprint) const override;
    // 单事件模式：更新事件
    bool updateEvent(const AlertEvent& event) override;

private:
    std::shared_ptr<pqxx::connection> conn_;
    mutable std::mutex mu_;
    
    // 辅助方法
    std::string parseDurationToInterval(const std::string& duration) const;
};

} // namespace alert
} // namespace yw
