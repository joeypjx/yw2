#include "alert_creation_factory.h"
#include "utils/json_config.h"
#include <spdlog/spdlog.h>
#include <sstream>

namespace yw {
namespace alert {

AlertCreationFactory::AlertCreationFactory(std::shared_ptr<AlertEventRepository> alertRepo,
                                           std::shared_ptr<DatabaseQueryInterface> dbInterface)
    : alertRepo_(alertRepo), dbInterface_(dbInterface),
      aliveCheckRunning_(false), aliveCheckShouldStop_(false),
      aliveCheckIntervalSeconds_(5),
      heartbeatTimeoutSeconds_(yw::utils::JsonConfig::Get<int>("alert.heartbeat_timeout_seconds", 5)) {
    if (!alertRepo_) {
        throw std::invalid_argument("AlertEventRepository不能为空");
    }
    if (!dbInterface_) {
        throw std::invalid_argument("DatabaseQueryInterface不能为空");
    }
}

AlertCreationFactory::~AlertCreationFactory() {
    stopAliveCheck();
}

void AlertCreationFactory::setPushCallback(std::function<void(const AlertEvent&)> callback) {
    pushCallback_ = std::move(callback);
}

std::shared_ptr<AlertEvent> AlertCreationFactory::createAlertFromComponent(const std::string& hostIp,
                                                              const std::string& instanceId,
                                                              const std::string& uuid,
                                                              int index,
                                                              const std::string& status,
                                                              const std::string& stack_name,
                                                              const std::string& component_name) {
    try {
        // 生成指纹（与AlertRoutes中的逻辑保持一致）
        std::string fingerprint = "业务组件状态异常|host_ip=" + hostIp + "|instance_id=" + instanceId + "|uuid=" + uuid + "|index=" + std::to_string(index);
        
        // 检查是否已存在相同的告警
        auto existingAlert = alertRepo_->getAlertByFingerprint(fingerprint);
        if (existingAlert) {
            // 如果已存在，更新现有告警
            existingAlert->setStatus(AlertStatus::Firing);
            existingAlert->setUpdatedNow();
            existingAlert->setStartsAt(existingAlert->getUpdatedAt());
            existingAlert->setEndsAt(""); // 清空结束时间 
            
            // 更新描述
            std::string description = hostIp + " 节点上业务(" + stack_name + ") 组件(" + component_name + ") 状态为 " + status;
            existingAlert->addLabel("description", description);
            
            // 保存到数据库
            bool success = existingAlert->updateInDatabase(alertRepo_);
            if (success) {
                spdlog::debug("更新现有组件告警: {}", fingerprint);
                
                // 组件告警直接设为 Firing 状态，需要推送
                if (pushCallback_) {
                    pushCallback_(*existingAlert);
                }
                
                return existingAlert;
            } else {
                spdlog::error("更新现有组件告警失败: {}", fingerprint);
                return nullptr;
            }
        }
        
        // 创建新组件告警的标签和注释
        std::unordered_map<std::string, std::string> alertLabels;
        std::unordered_map<std::string, std::string> alertAnnotations;

        // 设置基本标签
        alertLabels["alert_name"] = "业务组件状态异常";
        alertLabels["alert_type"] = "业务链路";
        alertLabels["severity"] = "严重";
        alertLabels["host_ip"] = hostIp;
        alertLabels["instance_id"] = instanceId;
        alertLabels["uuid"] = uuid;
        alertLabels["index"] = std::to_string(index);
        alertLabels["value"] = status;
        alertLabels["stack_name"] = stack_name;
        alertLabels["component_name"] = component_name;
        
        // 设置描述
        std::string description = hostIp + " 节点上业务(" + stack_name + ") 组件(" + component_name + ") 状态为 " + status;
        alertAnnotations["summary"] = "业务组件状态异常";
        alertAnnotations["description"] = description;
        
        // 创建新组件告警
        AlertEvent newAlert(fingerprint, alertLabels, alertAnnotations);
        newAlert.setStatus(AlertStatus::Firing);
        newAlert.setStartsAt(newAlert.getCreatedAt());
        newAlert.setEndsAt("");
        
        // 保存到数据库
        bool success = newAlert.updateInDatabase(alertRepo_);
        if (success) {
            spdlog::info("成功创建组件告警: {} (组件: {})", fingerprint, instanceId);
            
            // 组件告警直接设为 Firing 状态，需要推送
            if (pushCallback_) {
                pushCallback_(newAlert);
            }
            
            return std::make_shared<AlertEvent>(newAlert);
        } else {
            spdlog::error("创建组件告警失败: {}", fingerprint);
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        spdlog::error("创建组件告警时发生错误: {}", e.what());
        return nullptr;
    }
}

std::shared_ptr<AlertEvent> AlertCreationFactory::createBoardTypeChangeAlert(int box_id, int slot_id,
                                                                const std::string& cached_board_type,
                                                                const std::string& new_board_type) {
    try {
        // 生成指纹（使用 box_id 和 slot_id）
        std::string fingerprint = "节点板卡类型变化|box_id=" + std::to_string(box_id) + "|slot_id=" + std::to_string(slot_id);
        
        // 创建新板卡类型变化告警的标签和注释
        std::unordered_map<std::string, std::string> alertLabels;
        std::unordered_map<std::string, std::string> alertAnnotations;
        
        // 设置基本标签
        alertLabels["alert_name"] = "节点板卡类型变化";
        alertLabels["alert_type"] = "系统告警";
        alertLabels["severity"] = "警告";
        alertLabels["box_id"] = std::to_string(box_id);
        alertLabels["slot_id"] = std::to_string(slot_id);
        alertLabels["cached_board_type"] = cached_board_type;
        alertLabels["new_board_type"] = new_board_type;
        
        // 设置描述
        std::string description = "机箱 " + std::to_string(box_id) + " 槽位 " + std::to_string(slot_id) + 
                                " 的板卡类型从 " + cached_board_type + " 变为 " + new_board_type;
        alertAnnotations["summary"] = "节点板卡类型变化";
        alertAnnotations["description"] = description;
        
        // 创建新板卡类型变化告警
        AlertEvent newAlert(fingerprint, alertLabels, alertAnnotations);
        newAlert.setStatus(AlertStatus::Firing);
        newAlert.setStartsAt(newAlert.getCreatedAt());
        newAlert.setEndsAt(newAlert.getCreatedAt()); // 已解决状态，结束时间等于创建时间
        
        // 保存到数据库
        bool success = newAlert.updateInDatabase(alertRepo_);
        if (success) {
            spdlog::info("成功创建板卡类型变化告警: {} (机箱: {}, 槽位: {})", fingerprint, box_id, slot_id);
            
            // 推送告警
            if (pushCallback_) {
                pushCallback_(newAlert);
            }
            
            return std::make_shared<AlertEvent>(newAlert);
        } else {
            spdlog::error("创建板卡类型变化告警失败: {}", fingerprint);
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        spdlog::error("创建板卡类型变化告警时发生错误: {}", e.what());
        return nullptr;
    }
}

void AlertCreationFactory::startAliveCheck(int intervalSeconds) {
    if (aliveCheckRunning_) {
        spdlog::debug("节点存活检查已经在运行中");
        return;
    }
    
    aliveCheckIntervalSeconds_ = intervalSeconds;
    aliveCheckShouldStop_ = false;
    
    // 启动工作线程
    aliveCheckRunning_ = true;
    aliveCheckThread_ = std::thread(&AlertCreationFactory::aliveCheckWorkerLoop, this);
    
    spdlog::debug("节点存活检查已启动，检查间隔: {} 秒", aliveCheckIntervalSeconds_);
}

void AlertCreationFactory::stopAliveCheck() {
    if (!aliveCheckRunning_) {
        return;
    }
    
    spdlog::debug("正在停止节点存活检查...");
    aliveCheckShouldStop_ = true;
    
    if (aliveCheckThread_.joinable()) {
        aliveCheckThread_.join();
    }
    
    aliveCheckRunning_ = false;
    spdlog::debug("节点存活检查已停止");
}

void AlertCreationFactory::aliveCheckWorkerLoop() {
    spdlog::debug("节点存活检查工作线程已启动");
    
    while (!aliveCheckShouldStop_) {
        try {
            // 执行节点存活检查
            performEvaluationForAlive();
            
            // 可中断的等待：分段 sleep，每次检查停止标志
            for (int i = 0; i < aliveCheckIntervalSeconds_ * 10 && !aliveCheckShouldStop_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
        } catch (const std::exception& e) {
            spdlog::error("节点存活检查过程中发生错误: {}", e.what());
            // 发生错误时等待一段时间再继续（可中断）
            for (int i = 0; i < aliveCheckIntervalSeconds_ * 10 && !aliveCheckShouldStop_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    spdlog::debug("节点存活检查工作线程已退出");
}

int AlertCreationFactory::performEvaluationForAlive() {
    auto startTime = std::chrono::system_clock::now();
    
    try {
        spdlog::debug("=== 开始检查节点存活状态 ===");
        
        // 查询每个节点 IP 的最新时间，并在数据库层面直接计算时间差（秒）
        // 这样避免了时区转换的复杂性
        std::string sql = R"(
            SELECT 
                host(host_ip) as host_ip, 
                MAX(time) as latest_time,
                EXTRACT(EPOCH FROM (NOW() - MAX(time)))::int as seconds_since_last_alive
            FROM resource_alive 
            GROUP BY host_ip 
            ORDER BY host_ip
        )";
        
        QueryResult result = dbInterface_->executeQuery(sql);
        
        spdlog::debug("找到 {} 个节点", result.size());
        
        int alertCount = 0;
        
        // 遍历每个节点，检查最新时间
        for (const auto& row : result.rows) {
            std::string hostIp = row.getValue("host_ip");
            std::string latestTimeStr = row.getValue("latest_time");
            std::string secondsSinceLastAliveStr = row.getValue("seconds_since_last_alive");
            
            if (latestTimeStr.empty() || secondsSinceLastAliveStr.empty()) {
                spdlog::debug("节点 {}: 无记录", hostIp);
                continue;
            }
            
            // 从数据库查询结果中获取时间差（秒）
            int secondsSinceLastAlive = 0;
            try {
                secondsSinceLastAlive = std::stoi(secondsSinceLastAliveStr);
            } catch (const std::exception& e) {
                spdlog::error("节点 {}: 解析时间差失败: {}", hostIp, secondsSinceLastAliveStr);
                continue;
            }
            
            std::unordered_map<std::string, std::string> fingerprintTags;
            fingerprintTags["host_ip"] = hostIp;
            std::string fingerprint = AlertEvent::generateFingerprint("节点心跳超时", fingerprintTags);

            // 如果超过配置的阈值未心跳，创建 firing 告警
            if (secondsSinceLastAlive > heartbeatTimeoutSeconds_) {
                try {
                    // 检查是否已存在相同 fingerprint 的 firing 告警
                    auto existingAlerts = alertRepo_->getAlertsByFingerprintAndStatus(fingerprint, "firing");
                    
                    if (existingAlerts.size() > 0) {
                        // do not do anything
                    } else {
                        // 创建新告警的标签和注释
                        std::unordered_map<std::string, std::string> alertLabels;
                        std::unordered_map<std::string, std::string> alertAnnotations;
                        
                        // 设置基本标签
                        alertLabels["alert_name"] = "节点心跳超时";
                        alertLabels["alert_type"] = "系统告警";
                        alertLabels["severity"] = "严重";
                        alertLabels["host_ip"] = hostIp;
                        
                        // 设置描述
                        std::string description = "节点 " + hostIp + " 心跳超时，距离最新心跳已 " + 
                                                std::to_string(secondsSinceLastAlive) + " 秒";
                        alertLabels["description"] = description;
                        alertLabels["summary"] = "节点心跳超时";
                        
                        // 设置注释
                        alertAnnotations["description"] = description;
                        alertAnnotations["summary"] = "节点 " + hostIp + " 心跳超时";
                        alertAnnotations["monitoring_source"] = "alive_check";
                        alertAnnotations["last_alive_time"] = latestTimeStr;
                        alertAnnotations["seconds_since_last_alive"] = std::to_string(secondsSinceLastAlive);
                        
                        // 创建新告警
                        AlertEvent newAlert(fingerprint, alertLabels, alertAnnotations);
                        newAlert.setStatus(AlertStatus::Firing);
                        newAlert.setStartsAt(newAlert.getCreatedAt());
                        newAlert.setEndsAt("");
                        
                        // 保存到数据库
                        bool success = newAlert.updateInDatabase(alertRepo_);
                        if (success) {
                            spdlog::info("成功创建节点心跳超时告警: {} (指纹: {})", hostIp, fingerprint);
                            alertCount++;
                            
                            // 节点心跳超时告警直接设为 Firing 状态，需要推送
                            if (pushCallback_) {
                                pushCallback_(newAlert);
                            }
                        } else {
                            spdlog::error("创建节点心跳超时告警失败: {}", hostIp);
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::error("处理节点心跳超时告警时发生错误: {} - {}", hostIp, e.what());
                }
            } else {
                // 如果节点正常（<= 5秒），检查是否有已存在的告警需要解决
                try {
                    int resolvedCount = alertRepo_->resolveFiringAlertsByFingerprint(fingerprint);
                    if (resolvedCount > 0) {
                        spdlog::info("已解决 {} 个节点心跳超时告警: {}", resolvedCount, hostIp);
                    }
                } catch (const std::exception& e) {
                    spdlog::error("处理节点恢复时发生错误: {} - {}", hostIp, e.what());
                }
            }
        }
        
        auto endTime = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        spdlog::debug("节点存活状态检查完成，耗时: {} 毫秒，创建/更新了 {} 个告警", duration, alertCount);
        
        return result.size();
        
    } catch (const std::exception& e) {
        spdlog::error("检查节点存活状态失败: {}", e.what());
        throw;
    }
}

} // namespace alert
} // namespace yw

