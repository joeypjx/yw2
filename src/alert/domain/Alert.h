#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include "alert/alert_model.h"  // 使用公共接口的 AlertStatus 定义

namespace yw {
namespace alert {

// 前向声明
class AlertRepository;

// 告警类
class Alert {
public:
    Alert() = default;
    Alert(const std::string& fingerprint, const std::unordered_map<std::string, std::string>& labels,
          const std::unordered_map<std::string, std::string>& annotations);
    
    // 获取器方法
    const std::string& getId() const { return id_; }
    const std::string& getFingerprint() const { return fingerprint_; }
    const std::unordered_map<std::string, std::string>& getLabels() const { return labels_; }
    const std::unordered_map<std::string, std::string>& getAnnotations() const { return annotations_; }
    const std::string& getCreatedAt() const { return created_at_; }
    const std::string& getStartsAt() const { return starts_at_; }
    const std::string& getUpdatedAt() const { return updated_at_; }
    const std::string& getEndsAt() const { return ends_at_; }
    AlertStatus getStatus() const { return status_; }
    
    // 设置器方法
    void setId(const std::string& id) { id_ = id; }
    void setFingerprint(const std::string& fingerprint) { fingerprint_ = fingerprint; }
    void setLabels(const std::unordered_map<std::string, std::string>& labels) { labels_ = labels; }
    void setAnnotations(const std::unordered_map<std::string, std::string>& annotations) { annotations_ = annotations; }
    void setCreatedAt(const std::string& createdAt) { created_at_ = createdAt; }
    void setStartsAt(const std::string& startsAt) { starts_at_ = startsAt; }
    void setUpdatedAt(const std::string& updatedAt) { updated_at_ = updatedAt; }
    void setEndsAt(const std::string& endsAt) { ends_at_ = endsAt; }
    void setStatus(AlertStatus status) { status_ = status; }
    
    // 系统生成方法
    void generateId();
    void setCreatedNow();
    void setUpdatedNow();
    void setStartsNow();
    void setEndsNow();
    
    // 状态管理方法
    void transitionToPending();
    void transitionToFiring();
    void transitionToResolved();
    
    // 标签和注释管理
    void addLabel(const std::string& key, const std::string& value);
    void addAnnotation(const std::string& key, const std::string& value);
    std::string getLabel(const std::string& key) const;
    std::string getAnnotation(const std::string& key) const;
    
    // JSON序列化方法
    nlohmann::json toJson() const;
    static Alert fromJson(const nlohmann::json& j);
    
    // 工具方法
    static std::string generateFingerprint(const std::string& alertName, 
                                          const std::unordered_map<std::string, std::string>& tags);
    void updateTimestamp();
    
    // 数据库更新方法
    bool updateInDatabase(std::shared_ptr<AlertRepository> repository);

private:
    std::string id_;                                                      // 本次告警的ID，系统自动生成
    std::string fingerprint_;                                             // 告警指纹
    std::unordered_map<std::string, std::string> labels_;                 // 告警的全量标签
    std::unordered_map<std::string, std::string> annotations_;            // 告警通知的内容
    std::string created_at_;                                              // 第一次匹配时间
    std::string starts_at_;                                               // 第一次正式触发告警时间
    std::string updated_at_;                                              // 触发中持续匹配的更新时间
    std::string ends_at_;                                                 // 告警已解决的时间
    AlertStatus status_ = AlertStatus::Pending;                          // 告警状态
    
    // 私有辅助方法
    bool updateExistingAlert(const Alert& existingAlert, std::shared_ptr<AlertRepository> repository);
};

// JSON序列化支持
inline void to_json(nlohmann::json& j, const Alert& a) {
    j = nlohmann::json{
        {"id", a.getId()},
        {"fingerprint", a.getFingerprint()},
        {"labels", a.getLabels()},
        {"annotations", a.getAnnotations()},
        {"created_at", a.getCreatedAt()},
        {"starts_at", a.getStartsAt()},
        {"updated_at", a.getUpdatedAt()},
        {"ends_at", a.getEndsAt()},
        {"status", a.getStatus()}
    };
}

inline void from_json(const nlohmann::json& j, Alert& a) {
    a = Alert::fromJson(j);
}

} // namespace alert
} // namespace yw
