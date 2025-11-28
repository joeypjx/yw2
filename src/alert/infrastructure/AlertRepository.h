#pragma once

#include "../domain/Alert.h"
#include "DatabaseQueryInterface.h"
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

/**
 * @brief 告警查询过滤条件结构体
 */
struct AlertFilters {
    std::string status;              // 状态过滤 (pending/firing/resolved)
    std::string severity;            // 严重程度过滤
    std::string alert_type;          // 告警类型过滤
    std::string host_ip;             // 主机IP过滤
    int box_id = -1;                 // 机箱号过滤 (-1 表示不过滤)
    int slot_id = -1;                // 板卡号过滤 (-1 表示不过滤)
    std::string start_time;          // 起始时间过滤
    std::string end_time;            // 结束时间过滤
    std::string description;         // 描述过滤（模糊匹配）
    int limit = 100;                 // 限制返回数量 (默认100, 最大1000)
    std::string stack_name;          // 栈名过滤
    std::string component_name;      // 组件名过滤
    
    // 检查是否有任何过滤条件
    bool hasAnyFilter() const {
        return !status.empty() || !severity.empty() || !alert_type.empty() ||
               !host_ip.empty() || box_id >= 0 || slot_id >= 0 ||
               !start_time.empty() || !end_time.empty() || !description.empty() ||
               !stack_name.empty() || !component_name.empty();
    }
};

class AlertRepository {
public:
    virtual ~AlertRepository() = default;
    
    virtual bool saveAlert(const Alert& alert) = 0;
    virtual std::shared_ptr<Alert> getAlertById(const std::string& id) = 0;
    virtual std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint) = 0;
    virtual std::vector<Alert> getAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) = 0;
    virtual bool deleteAlert(const std::string& id) = 0;
    virtual int deleteAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) = 0;
    virtual int resolveFiringAlertsByFingerprint(const std::string& fingerprint) = 0;
    virtual bool alertExists(const std::string& id) = 0;
    
    virtual std::vector<Alert> getAlertsByStatus(const std::string& status) = 0;
    virtual std::vector<Alert> getAlertsByHostIp(const std::string& hostIp) = 0;
    virtual std::vector<Alert> getAlertsByBoxId(int boxId) = 0;
    virtual std::vector<Alert> getAlertsBySlotId(int slotId) = 0;
    virtual std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) = 0;
    virtual std::vector<Alert> getAlertsByAlertType(const std::string& alertType) = 0;
    virtual std::vector<Alert> getAlertsBySeverity(const std::string& severity) = 0;
    virtual std::vector<Alert> getAlertsByDescription(const std::string& description) = 0;
    virtual std::vector<Alert> getAlertsExceptPending() = 0;
    
    // 统一的过滤查询接口，支持多个过滤条件组合
    virtual std::vector<Alert> getAlertsByFilters(const AlertFilters& filters) = 0;
    
    virtual size_t getAlertCount() = 0;
};

class DatabaseAlertRepository : public AlertRepository {
public:
    explicit DatabaseAlertRepository(std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~DatabaseAlertRepository() override = default;
    
    bool saveAlert(const Alert& alert) override;
    std::shared_ptr<Alert> getAlertById(const std::string& id) override;
    std::shared_ptr<Alert> getAlertByFingerprint(const std::string& fingerprint) override;
    std::vector<Alert> getAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) override;
    std::vector<Alert> getAlertsByStatus(const std::string& status) override;
    std::vector<Alert> getAlertsByHostIp(const std::string& hostIp) override;
    std::vector<Alert> getAlertsByBoxId(int boxId) override;
    std::vector<Alert> getAlertsBySlotId(int slotId) override;
    std::vector<Alert> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) override;
    std::vector<Alert> getAlertsByAlertType(const std::string& alertType) override;
    std::vector<Alert> getAlertsBySeverity(const std::string& severity) override;
    std::vector<Alert> getAlertsByDescription(const std::string& description) override;
    std::vector<Alert> getAlertsExceptPending() override;
    std::vector<Alert> getAlertsByFilters(const AlertFilters& filters) override;
    bool deleteAlert(const std::string& id) override;
    int deleteAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) override;
    int resolveFiringAlertsByFingerprint(const std::string& fingerprint) override;
    bool alertExists(const std::string& id) override;
    size_t getAlertCount() override;

private:
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    
    Alert parseAlertFromQueryResult(const QueryRow& row);
    std::string buildInsertSql();
    std::string buildUpdateSql();
    std::string buildSelectSql();
    std::string buildDeleteSql();
    
    // 构建动态 WHERE 子句和参数
    std::pair<std::string, std::vector<std::string>> buildWhereClause(const AlertFilters& filters);
};

} // namespace alert
} // namespace yw
