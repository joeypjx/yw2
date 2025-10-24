#pragma once

#include "../domain/Alert.h"
#include "DatabaseQueryInterface.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alertv2 {

/**
 * @brief 告警存储抽象接口
 * 
 * 定义了告警存储的所有操作接口，支持不同的存储实现
 */
class AlertRepository {
public:
    virtual ~AlertRepository() = default;
    
    // 基本CRUD操作
    virtual bool saveAlert(const Alert& alert) = 0;
    virtual std::shared_ptr<Alert> getAlertById(const std::string& id) = 0;
    virtual std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint) = 0;
    virtual bool deleteAlert(const std::string& id) = 0;
    virtual bool alertExists(const std::string& id) = 0;
    
    // 查询操作
    virtual std::vector<Alert> getAllAlerts() = 0;
    virtual std::vector<Alert> getAlertsByStatus(const std::string& status) = 0;
    virtual std::vector<Alert> getAlertsByHostIp(const std::string& hostIp) = 0;
    virtual std::vector<Alert> getAlertsByRuleId(const std::string& ruleId) = 0;
    virtual std::vector<Alert> getAlertsByAlertName(const std::string& alertName) = 0;
    virtual std::vector<Alert> getAlertsByAlertType(const std::string& alertType) = 0;
    virtual std::vector<Alert> getAlertsBySeverity(const std::string& severity) = 0;
    
    // 状态相关查询
    virtual std::vector<Alert> getActiveAlerts() = 0;
    virtual std::vector<Alert> getPendingAlerts() = 0;
    virtual std::vector<Alert> getResolvedAlerts() = 0;
    virtual std::vector<Alert> getAlertsExceptPending() = 0;
    
    // 统计操作
    virtual size_t getAlertCount() = 0;
    virtual size_t getAlertCountByStatus(const std::string& status) = 0;
    virtual size_t getAlertCountByHostIp(const std::string& hostIp) = 0;
    virtual size_t getAlertCountByRuleId(const std::string& ruleId) = 0;
    virtual size_t getAlertCountByAlertName(const std::string& alertName) = 0;
    virtual size_t getAlertCountByAlertType(const std::string& alertType) = 0;
    virtual size_t getAlertCountBySeverity(const std::string& severity) = 0;
    virtual size_t getAlertCountByTimeRange(const std::string& startTime, const std::string& endTime) = 0;
    virtual size_t getActiveAlertCount() = 0;
    virtual size_t getPendingAlertCount() = 0;
    virtual size_t getResolvedAlertCount() = 0;
    
    // 批量操作
    virtual bool saveAlerts(const std::vector<Alert>& alerts) = 0;
    virtual bool deleteAlerts(const std::vector<std::string>& ids) = 0;
    
    // 更新操作
    virtual bool updateAlertStatus(const std::string& id, const std::string& status) = 0;
    virtual bool updateAlertTimestamps(const std::string& id, 
                                      const std::string& startsAt = "",
                                      const std::string& updatedAt = "",
                                      const std::string& endsAt = "") = 0;
    virtual bool resolveAlert(const std::string& id) = 0;
    
    // 时间范围查询
    virtual std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, 
                                                   const std::string& endTime) = 0;
    virtual std::vector<Alert> getRecentAlerts(size_t count) = 0;
    
    // 清理操作
    virtual bool cleanupExpiredAlerts(const std::string& beforeTime) = 0;
};

/**
 * @brief 基于PostgreSQL的告警存储实现
 * 
 * 使用PostgreSQL数据库存储告警信息，支持JSON字段存储标签和注释
 */
class DatabaseAlertRepository : public AlertRepository {
public:
    explicit DatabaseAlertRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~DatabaseAlertRepository() override = default;
    
    // 实现AlertRepository接口
    bool saveAlert(const Alert& alert) override;
    std::shared_ptr<Alert> getAlertById(const std::string& id) override;
    std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint) override;
    std::vector<Alert> getAllAlerts() override;
    std::vector<Alert> getAlertsByStatus(const std::string& status) override;
    std::vector<Alert> getAlertsByHostIp(const std::string& hostIp) override;
    std::vector<Alert> getAlertsByRuleId(const std::string& ruleId) override;
    std::vector<Alert> getAlertsByAlertName(const std::string& alertName) override;
    std::vector<Alert> getAlertsByAlertType(const std::string& alertType) override;
    std::vector<Alert> getAlertsBySeverity(const std::string& severity) override;
    std::vector<Alert> getActiveAlerts() override;
    std::vector<Alert> getPendingAlerts() override;
    std::vector<Alert> getResolvedAlerts() override;
    std::vector<Alert> getAlertsExceptPending() override;
    bool deleteAlert(const std::string& id) override;
    bool alertExists(const std::string& id) override;
    size_t getAlertCount() override;
    size_t getAlertCountByStatus(const std::string& status) override;
    size_t getAlertCountByHostIp(const std::string& hostIp) override;
    size_t getAlertCountByRuleId(const std::string& ruleId) override;
    size_t getAlertCountByAlertName(const std::string& alertName) override;
    size_t getAlertCountByAlertType(const std::string& alertType) override;
    size_t getAlertCountBySeverity(const std::string& severity) override;
    size_t getAlertCountByTimeRange(const std::string& startTime, const std::string& endTime) override;
    size_t getActiveAlertCount() override;
    size_t getPendingAlertCount() override;
    size_t getResolvedAlertCount() override;
    bool saveAlerts(const std::vector<Alert>& alerts) override;
    bool deleteAlerts(const std::vector<std::string>& ids) override;
    bool updateAlertStatus(const std::string& id, const std::string& status) override;
    bool updateAlertTimestamps(const std::string& id, 
                              const std::string& startsAt = "",
                              const std::string& updatedAt = "",
                              const std::string& endsAt = "") override;
    bool resolveAlert(const std::string& id) override;
    std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, 
                                           const std::string& endTime) override;
    std::vector<Alert> getRecentAlerts(size_t count) override;
    bool cleanupExpiredAlerts(const std::string& beforeTime) override;

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    // 辅助方法
    Alert parseAlertFromQueryResult(const QueryRow& row);
    std::string buildInsertSql();
    std::string buildUpdateSql();
    std::string buildSelectSql();
    std::string buildDeleteSql();
    std::string escapeString(const std::string& str);
    std::string formatTimestamp(const std::string& timestamp);
};

} // namespace alertv2
} // namespace yw
