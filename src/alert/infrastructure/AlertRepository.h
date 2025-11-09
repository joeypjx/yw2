#pragma once

#include "../domain/Alert.h"
#include "DatabaseQueryInterface.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

class AlertRepository {
public:
    virtual ~AlertRepository() = default;
    
    virtual bool saveAlert(const Alert& alert) = 0;
    virtual std::shared_ptr<Alert> getAlertById(const std::string& id) = 0;
    virtual std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint) = 0;
    virtual bool deleteAlert(const std::string& id) = 0;
    virtual bool alertExists(const std::string& id) = 0;
    
    virtual std::vector<Alert> getAlertsByStatus(const std::string& status) = 0;
    virtual std::vector<Alert> getAlertsByHostIp(const std::string& hostIp) = 0;
    virtual std::vector<Alert> getAlertsByBoxId(int boxId) = 0;
    virtual std::vector<Alert> getAlertsBySlotId(int slotId) = 0;
    virtual std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) = 0;
    virtual std::vector<Alert> getAlertsByAlertType(const std::string& alertType) = 0;
    virtual std::vector<Alert> getAlertsBySeverity(const std::string& severity) = 0;
    virtual std::vector<Alert> getAlertsExceptPending() = 0;
    
    virtual size_t getAlertCount() = 0;
};

class DatabaseAlertRepository : public AlertRepository {
public:
    explicit DatabaseAlertRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~DatabaseAlertRepository() override = default;
    
    bool saveAlert(const Alert& alert) override;
    std::shared_ptr<Alert> getAlertById(const std::string& id) override;
    std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint) override;
    std::vector<Alert> getAlertsByStatus(const std::string& status) override;
    std::vector<Alert> getAlertsByHostIp(const std::string& hostIp) override;
    std::vector<Alert> getAlertsByBoxId(int boxId) override;
    std::vector<Alert> getAlertsBySlotId(int slotId) override;
    std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) override;
    std::vector<Alert> getAlertsByAlertType(const std::string& alertType) override;
    std::vector<Alert> getAlertsBySeverity(const std::string& severity) override;
    std::vector<Alert> getAlertsExceptPending() override;
    bool deleteAlert(const std::string& id) override;
    bool alertExists(const std::string& id) override;
    size_t getAlertCount() override;

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    Alert parseAlertFromQueryResult(const QueryRow& row);
    std::string buildInsertSql();
    std::string buildUpdateSql();
    std::string buildSelectSql();
    std::string buildDeleteSql();
};

} // namespace alert
} // namespace yw
