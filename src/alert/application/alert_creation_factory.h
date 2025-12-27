// ============================================================================
// 文件功能描述：
// 告警创建工厂（AlertCreationFactory）的头文件，定义告警事件创建功能的接口。
// 主要功能包括：
// 1. 组件告警创建：从业务组件状态创建告警（如容器异常、进程异常等）
// 2. 板卡类型变化告警：检测节点板卡类型变化，创建相应的告警事件
// 3. 心跳告警创建：检测节点心跳超时，创建节点离线告警
// 4. 心跳检查：定期检查节点心跳状态，自动创建或更新心跳告警
// 5. 告警去重：使用告警指纹（fingerprint）避免重复创建相同的告警
// 6. 回调通知：创建或更新告警时，调用推送回调函数通知外部系统
// 7. 线程管理：使用独立线程运行心跳检查循环，支持优雅启动和停止
// ============================================================================

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

