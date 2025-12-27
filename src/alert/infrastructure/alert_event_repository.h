#pragma once

#include "../domain/alert_event.h"
#include "database_query_interface.h"
#include "alert/alert_model.h"  // 使用公共接口的 AlertFilters 定义
#include <vector>
#include <memory>
#include <string>

namespace yw {
namespace alert {

// 告警事件仓库接口，定义告警事件的持久化操作
class AlertEventRepository {
public:
    virtual ~AlertEventRepository() = default;
    
    // 保存告警事件（插入或更新）
    virtual bool saveAlert(const AlertEvent& alert) = 0;
    // 根据ID获取告警事件
    virtual std::shared_ptr<AlertEvent> getAlertById(const std::string& id) = 0;
    // 根据指纹获取告警事件
    virtual std::shared_ptr<AlertEvent> getAlertByFingerprint(const std::string& fingerprint) = 0;
    // 根据指纹和状态获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) = 0;
    // 删除指定ID的告警事件
    virtual bool deleteAlert(const std::string& id) = 0;
    // 删除指定指纹和状态的所有告警事件
    virtual int deleteAlertsByFingerprintAndStatus(const std::string& fingerprint, const std::string& status) = 0;
    // 将指定指纹的所有Firing状态告警标记为Resolved
    virtual int resolveFiringAlertsByFingerprint(const std::string& fingerprint) = 0;
    // 检查告警事件是否存在
    virtual bool alertExists(const std::string& id) = 0;
    
    // 根据状态获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByStatus(const std::string& status) = 0;
    // 根据状态和类型获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByStatusAndType(const std::string& status, const std::string& alertType) = 0;
    // 根据主机IP获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByHostIp(const std::string& hostIp) = 0;
    // 根据机箱号获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByBoxId(int boxId) = 0;
    // 根据槽位号获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsBySlotId(int slotId) = 0;
    // 根据时间范围获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByTimeRange(const std::string& startTime, const std::string& endTime) = 0;
    // 根据告警类型获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsByAlertType(const std::string& alertType) = 0;
    // 根据严重程度获取告警事件列表
    virtual std::vector<AlertEvent> getAlertsBySeverity(const std::string& severity) = 0;
    // 根据描述获取告警事件列表（模糊匹配）
    virtual std::vector<AlertEvent> getAlertsByDescription(const std::string& description) = 0;
    // 获取除Pending状态外的所有告警事件
    virtual std::vector<AlertEvent> getAlertsExceptPending() = 0;
    
    // 统一的过滤查询接口，支持多个过滤条件组合
    virtual std::vector<AlertEvent> getAlertsByFilters(const AlertFilters& filters) = 0;
    
    // 获取告警事件总数
    virtual size_t getAlertCount() = 0;
};

// 基于数据库的告警事件仓库实现
class DatabaseAlertEventRepository : public AlertEventRepository {
public:
    // 构造函数，初始化数据库查询接口
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
