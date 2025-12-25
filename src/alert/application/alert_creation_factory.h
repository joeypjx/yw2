#pragma once

#include "../domain/alert_event.h"
#include "../infrastructure/alert_event_repository.h"
#include "../infrastructure/database_query_interface.h"
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

namespace yw {
namespace alert {

/**
 * @brief 告警创建工厂类
 * 
 * 负责创建各种类型的告警，包括业务组件告警、板卡类型变化告警等
 * 同时负责节点存活状态检查
 */
class AlertCreationFactory {
public:
    AlertCreationFactory(std::shared_ptr<AlertEventRepository> alertRepo,
                         std::shared_ptr<DatabaseQueryInterface> dbInterface);
    ~AlertCreationFactory();
    
    /**
     * @brief 设置推送回调函数
     */
    void setPushCallback(std::function<void(const AlertEvent&)> callback);
    
    /**
     * @brief 启动节点存活检查线程
     * @param intervalSeconds 检查间隔（秒），默认5秒
     */
    void startAliveCheck(int intervalSeconds = 5);
    
    /**
     * @brief 停止节点存活检查线程
     */
    void stopAliveCheck();
    
    /**
     * @brief 创建业务组件状态异常告警
     */
    std::shared_ptr<AlertEvent> createAlertFromComponent(const std::string& hostIp,
                                                     const std::string& instanceId,
                                                     const std::string& uuid,
                                                     int index,
                                                     const std::string& status,
                                                     const std::string& stack_name,
                                                     const std::string& component_name);
    
    /**
     * @brief 创建节点板卡类型变化告警
     */
    std::shared_ptr<AlertEvent> createBoardTypeChangeAlert(int box_id, int slot_id,
                                                      const std::string& cached_board_type,
                                                      const std::string& new_board_type);

private:
    std::shared_ptr<AlertEventRepository> alertRepo_;
    std::shared_ptr<DatabaseQueryInterface> dbInterface_;
    std::function<void(const AlertEvent&)> pushCallback_;
    
    // 节点存活检查线程管理
    std::atomic<bool> aliveCheckRunning_;
    std::atomic<bool> aliveCheckShouldStop_;
    std::thread aliveCheckThread_;
    int aliveCheckIntervalSeconds_;
    int heartbeatTimeoutSeconds_;
    
    /**
     * @brief 节点存活检查工作循环
     */
    void aliveCheckWorkerLoop();
    
    /**
     * @brief 执行节点存活状态评估
     */
    int performEvaluationForAlive();
};

} // namespace alert
} // namespace yw

