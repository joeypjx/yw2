#include "Alert.h"
#include "../infrastructure/AlertRepository.h"
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <random>
#include <algorithm>

namespace yw {
namespace alertv2 {

Alert::Alert(const std::string& fingerprint, const std::unordered_map<std::string, std::string>& labels,
             const std::unordered_map<std::string, std::string>& annotations)
    : fingerprint_(fingerprint), labels_(labels), annotations_(annotations) {
    // 自动生成系统字段
    generateId();
    setCreatedNow();
    setUpdatedNow();
    // starts_at 和 ends_at 初始为空
    status_ = AlertStatus::Pending;
}

void Alert::generateId() {
    // 使用时间戳和随机数生成唯一ID
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    std::ostringstream oss;
    oss << "alert_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
        << "_" << ms.count() << "_" << dis(gen);
    
    id_ = oss.str();
}

void Alert::setCreatedNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    created_at_ = oss.str();
}

void Alert::setUpdatedNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    updated_at_ = oss.str();
}

void Alert::setStartsNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    starts_at_ = oss.str();
}

void Alert::setEndsNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    ends_at_ = oss.str();
}

void Alert::transitionToPending() {
    status_ = AlertStatus::Pending;
    setUpdatedNow();
    // 清空starts_at和ends_at
    starts_at_.clear();
    ends_at_.clear();
}

void Alert::transitionToFiring() {
    if (status_ == AlertStatus::Pending) {
        status_ = AlertStatus::Firing;
        setStartsNow();
        setUpdatedNow();
        // 清空ends_at
        ends_at_.clear();
    }
}

void Alert::transitionToResolved() {
    if (status_ == AlertStatus::Firing) {
        status_ = AlertStatus::Resolved;
        setEndsNow();
        setUpdatedNow();
    }
}

void Alert::addLabel(const std::string& key, const std::string& value) {
    labels_[key] = value;
}

void Alert::addAnnotation(const std::string& key, const std::string& value) {
    annotations_[key] = value;
}

std::string Alert::getLabel(const std::string& key) const {
    auto it = labels_.find(key);
    return (it != labels_.end()) ? it->second : "";
}

std::string Alert::getAnnotation(const std::string& key) const {
    auto it = annotations_.find(key);
    return (it != annotations_.end()) ? it->second : "";
}

nlohmann::json Alert::toJson() const {
    nlohmann::json j;
    to_json(j, *this);
    return j;
}

Alert Alert::fromJson(const nlohmann::json& j) {
    Alert alert;
    
    // 解析id
    if (j.contains("id")) {
        j.at("id").get_to(alert.id_);
    } else {
        alert.generateId();
    }
    
    // 解析fingerprint
    if (j.contains("fingerprint")) {
        j.at("fingerprint").get_to(alert.fingerprint_);
    }
    
    // 解析labels
    if (j.contains("labels")) {
        j.at("labels").get_to(alert.labels_);
    }
    
    // 解析annotations
    if (j.contains("annotations")) {
        j.at("annotations").get_to(alert.annotations_);
    }
    
    // 解析created_at
    if (j.contains("created_at")) {
        j.at("created_at").get_to(alert.created_at_);
    } else {
        alert.setCreatedNow();
    }
    
    // 解析starts_at
    if (j.contains("starts_at")) {
        j.at("starts_at").get_to(alert.starts_at_);
    }
    
    // 解析updated_at
    if (j.contains("updated_at")) {
        j.at("updated_at").get_to(alert.updated_at_);
    } else {
        alert.setUpdatedNow();
    }
    
    // 解析ends_at
    if (j.contains("ends_at")) {
        j.at("ends_at").get_to(alert.ends_at_);
    }
    
    // 解析status
    if (j.contains("status")) {
        alert.status_ = j.at("status").get<AlertStatus>();
    } else {
        alert.status_ = AlertStatus::Pending;
    }
    
    return alert;
}

std::string Alert::generateFingerprint(const std::string& alertName, 
                                      const std::unordered_map<std::string, std::string>& tags) {
    std::ostringstream oss;
    oss << alertName;
    
    // 按key排序以确保一致性
    std::vector<std::pair<std::string, std::string>> sortedTags(tags.begin(), tags.end());
    std::sort(sortedTags.begin(), sortedTags.end());
    
    for (const auto& tag : sortedTags) {
        oss << "|" << tag.first << "=" << tag.second;
    }
    
    return oss.str();
}

void Alert::updateTimestamp() {
    setUpdatedNow();
}

bool Alert::updateInDatabase(std::shared_ptr<AlertRepository> repository) {
    try {
        if (!repository) {
            throw std::invalid_argument("AlertRepository不能为空");
        }
        
        // 1. 先从告警存储类获取当前数据库中告警指纹所对应的最新告警
        auto existingAlert = repository->getAlertByFingerprint(fingerprint_);
        
        if (!existingAlert) {
            // 2. 如果数据库中没有这个告警指纹，则在数据库中增加这个告警
            return repository->saveAlert(*this);
        } else {
            // 3. 如果数据库中已经有这个告警指纹，需要判断现有告警的状态
            AlertStatus existingStatus = existingAlert->getStatus();
            AlertStatus newStatus = this->getStatus();
            
            // 如果现有告警是resolved状态，而新告警是pending或firing状态，则创建新告警
            if (existingStatus == AlertStatus::Resolved && 
                (newStatus == AlertStatus::Pending || newStatus == AlertStatus::Firing)) {
                // 创建新告警，不更新现有的resolved告警
                return repository->saveAlert(*this);
            } else {
                // 其他情况，更新现有告警
                return updateExistingAlert(*existingAlert, repository);
            }
        }
        
    } catch (const std::exception& e) {
        throw std::runtime_error("更新告警到数据库失败: " + std::string(e.what()));
    }
}

bool Alert::updateExistingAlert(const Alert& existingAlert, std::shared_ptr<AlertRepository> repository) {
    AlertStatus newStatus = this->getStatus();
    AlertStatus currentStatus = existingAlert.getStatus();
    
    if (newStatus != currentStatus) {
        // 状态发生变化，需要验证状态转换是否合法
        
        // 不允许的状态转换：
        // 1. Firing -> Pending (不能从触发状态退回到等待状态)
        if (currentStatus == AlertStatus::Firing && newStatus == AlertStatus::Pending) {
            // 保持原有状态，只更新时间戳
            Alert updatedAlert = existingAlert;
            updatedAlert.updateTimestamp();
            return repository->saveAlert(updatedAlert);
        }
        
        // 2. Resolved -> Pending (已解决的告警不应该退回到等待状态)
        // 这种情况应该创建新告警，已在 updateInDatabase 中处理
        
        // 允许的状态转换：
        // - Pending -> Firing
        // - Firing -> Resolved
        
        Alert updatedAlert = *this;
        
        // 保持原有的ID和时间戳信息
        updatedAlert.setId(existingAlert.getId());
        updatedAlert.setCreatedAt(existingAlert.getCreatedAt());
        
        // 根据状态转换设置相应的时间戳
        if (newStatus == AlertStatus::Firing && currentStatus != AlertStatus::Firing) {
            // 从非firing状态转为firing
            if (updatedAlert.getStartsAt().empty()) {
                updatedAlert.setStartsNow();
            }
            updatedAlert.setUpdatedNow();
            updatedAlert.setEndsAt(""); // 清空结束时间
        } else if (newStatus == AlertStatus::Resolved && currentStatus != AlertStatus::Resolved) {
            // 从非resolved状态转为resolved
            if (updatedAlert.getEndsAt().empty()) {
                updatedAlert.setEndsNow();
            }
            updatedAlert.setUpdatedNow();
        } else {
            // 其他状态变化，只更新updated_at
            updatedAlert.setUpdatedNow();
        }
        
        return repository->saveAlert(updatedAlert);
    } else {
        // 状态没有变化，检查是否需要从Pending转为Firing
        if (currentStatus == AlertStatus::Pending && newStatus == AlertStatus::Pending) {
            // 检查是否满足持续时间条件（这里需要传入AlertRule的for字段）
            // 由于当前Alert对象没有for字段信息，我们需要通过其他方式获取
            // 暂时只更新updated_at，持续时间检查在AlertEngine中处理
            Alert updatedAlert = existingAlert;
            updatedAlert.updateTimestamp();
            return repository->saveAlert(updatedAlert);
        } else {
            // 状态没有变化，只更新updated_at时间戳
            Alert updatedAlert = existingAlert;
            updatedAlert.updateTimestamp();
            return repository->saveAlert(updatedAlert);
        }
    }
}

} // namespace alertv2
} // namespace yw
