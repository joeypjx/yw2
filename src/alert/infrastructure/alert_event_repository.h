#pragma once

#include "../domain/alert_event.h"
#include "database_query_interface.h"
#include "alert/alert_model.h"  // 使用公共接口的 AlertFilters 定义
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

class AlertEventRepository {
public:
    virtual ~AlertEventRepository() = default;
    
    virtual bool saveAlert(const AlertEvent& alert) = 0;
    virtual std::shared_ptr<AlertEvent> getAlertById(const std::string& id) = 0;
    virtual std::shared_ptr<AlertEvent> getAlertByFingerprint(const std::string& fingerprint) = 0;
    virtual std::vector<AlertEvent> getAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) = 0;
    virtual bool deleteAlert(const std::string& id) = 0;
    virtual int deleteAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) = 0;
    virtual int resolveFiringAlertsByFingerprint(const std::string& fingerprint) = 0;
    virtual bool alertExists(const std::string& id) = 0;
    
    virtual std::vector<AlertEvent> getAlertsByStatus(const std::string& status) = 0;
    virtual std::vector<AlertEvent> getAlertsByStatusAndType(const std::string& status, const std::string& alertType) = 0;
    virtual std::vector<AlertEvent> getAlertsByHostIp(const std::string& hostIp) = 0;
    virtual std::vector<AlertEvent> getAlertsByBoxId(int boxId) = 0;
    virtual std::vector<AlertEvent> getAlertsBySlotId(int slotId) = 0;
    virtual std::vector<AlertEvent> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) = 0;
    virtual std::vector<AlertEvent> getAlertsByAlertType(const std::string& alertType) = 0;
    virtual std::vector<AlertEvent> getAlertsBySeverity(const std::string& severity) = 0;
    virtual std::vector<AlertEvent> getAlertsByDescription(const std::string& description) = 0;
    virtual std::vector<AlertEvent> getAlertsExceptPending() = 0;
    
    // 统一的过滤查询接口，支持多个过滤条件组合
    virtual std::vector<AlertEvent> getAlertsByFilters(const AlertFilters& filters) = 0;
    
    virtual size_t getAlertCount() = 0;
};

class DatabaseAlertEventRepository : public AlertEventRepository {
public:
    explicit DatabaseAlertEventRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~DatabaseAlertEventRepository() override = default;
    
    bool saveAlert(const AlertEvent& alert) override;
    std::shared_ptr<AlertEvent> getAlertById(const std::string& id) override;
    std::shared_ptr<AlertEvent> getAlertByFingerprint(const std::string& fingerprint) override;
    std::vector<AlertEvent> getAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) override;
    std::vector<AlertEvent> getAlertsByStatus(const std::string& status) override;
    std::vector<AlertEvent> getAlertsByStatusAndType(const std::string& status, const std::string& alertType) override;
    std::vector<AlertEvent> getAlertsByHostIp(const std::string& hostIp) override;
    std::vector<AlertEvent> getAlertsByBoxId(int boxId) override;
    std::vector<AlertEvent> getAlertsBySlotId(int slotId) override;
    std::vector<AlertEvent> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) override;
    std::vector<AlertEvent> getAlertsByAlertType(const std::string& alertType) override;
    std::vector<AlertEvent> getAlertsBySeverity(const std::string& severity) override;
    std::vector<AlertEvent> getAlertsByDescription(const std::string& description) override;
    std::vector<AlertEvent> getAlertsExceptPending() override;
    std::vector<AlertEvent> getAlertsByFilters(const AlertFilters& filters) override;
    bool deleteAlert(const std::string& id) override;
    int deleteAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) override;
    int resolveFiringAlertsByFingerprint(const std::string& fingerprint) override;
    bool alertExists(const std::string& id) override;
    size_t getAlertCount() override;

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    AlertEvent parseAlertFromQueryResult(const QueryRow& row);
    std::string buildInsertSql();
    std::string buildUpdateSql();
    std::string buildSelectSql();
    std::string buildDeleteSql();
    
    // 构建动态 WHERE 子句和参数
    std::pair<std::string, std::vector<std::string>> buildWhereClause(const AlertFilters& filters);
};

} // namespace alert
} // namespace yw
