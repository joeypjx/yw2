#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <optional>

#include <spdlog/spdlog.h>
#include <pqxx/pqxx>
#include <eventpp/eventdispatcher.h>

#include "yw/alert.h"
#include "AlertServices.h"
#include <functional>


namespace yw {
namespace alert {

// AlertManager 对外分发的事件类型（最小集）
enum class AlertManagerEvent {
    AlertEventAppended, // 产生/变更了一条 AlertEvent
};

// 调度器类型：回调签名统一传出 AlertEvent
using EventDispatcher = eventpp::EventDispatcher<AlertManagerEvent, void(const AlertEvent&)>;

class AlertManager : public IAlertModule {
public:
    AlertManager();
    ~AlertManager() override;

    // 规则管理
    std::vector<Rule> listRules() const override;
    std::optional<Rule> getRule(const std::string& id) const override;
    bool upsertRule(const Rule& rule) override;
    bool deleteRule(const std::string& id) override;

    // 告警查询与操作
    std::vector<AlertEvent> queryEvents(const std::string& duration) const override;
    std::size_t countEventsByStatus(AlertStatus status) const override;
    bool appendAlertEvent(const AlertEvent& event) override;
    bool ackAlert(const std::string& fingerprint, const std::string& user, const std::string& comment) override;

    // 事件调度器访问（订阅方可注册回调）
    EventDispatcher& dispatcher() { return dispatcher_; }

    // 设置外部推送回调（由 Web 层提供 push(event)）
    void setPushCallback(std::function<void(const AlertEvent&)> cb) override { push_cb_ = std::move(cb); }

private:
    std::shared_ptr<pqxx::connection> conn_;
    mutable std::mutex                mu_;

    // 内部服务聚合
    std::shared_ptr<IRuleRepository>       rule_repo_;
    std::shared_ptr<IAlertRepository>      alert_repo_;
    std::shared_ptr<IEventRepository>      event_repo_;
    std::shared_ptr<IFingerprintGenerator> fp_;
    std::shared_ptr<ITimeseriesProvider>   ts_;
    std::shared_ptr<IAlertEvaluator>       evaluator_;
    std::shared_ptr<IAlertStateManager>    state_manager_;
    std::shared_ptr<IScheduler>            scheduler_;

    // 事件分发器
    EventDispatcher dispatcher_;

    // 外部推送回调
    std::function<void(const AlertEvent&)> push_cb_;
};

} // namespace alert
} // namespace yw


