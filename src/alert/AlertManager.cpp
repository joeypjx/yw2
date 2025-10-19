// 1. 先包含 AlertManager.h
#include "AlertManager.h"
#include "AlertServices.h"  // 包含所有服务实现
#include "DatabaseEventRepository.h"
#include "DatabaseRuleRepository.h"
#include <chrono>
#include <cctype>
#include "yw/JsonConfig.h"
#include "yw/DurationUtils.h"

namespace yw {
namespace alert {


AlertManager::AlertManager() {
    // 创建数据库连接
    try {
        conn_ = std::make_shared<pqxx::connection>(yw::utils::JsonConfig::Get<std::string>("db.conninfo", "postgres://postgres:HZ715Net@localhost:5432/yw"));
        spdlog::info("Database connection established successfully");
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to database: {}", e.what());
        conn_ = nullptr;
    }
    
    // 组装最小运行所需服务（数据库实现 + 简易提供者）
    rule_repo_     = std::make_shared<DatabaseRuleRepository>(conn_);
    alert_repo_    = std::make_shared<MemoryAlertRepository>();
    event_repo_    = std::make_shared<DatabaseEventRepository>(conn_);
    // 注意：这里使用 DatabaseEventRepository，如果需要使用 MemoryEventRepository，可以这样创建：
    // event_repo_ = std::make_shared<MemoryEventRepository>(10000, 7 * 24 * 60 * 60 * 1000); // 10000个事件，7天
    fp_            = std::make_shared<SimpleFingerprintGenerator>();
    ts_            = std::make_shared<SimpleTimeseriesProvider>(conn_);
    evaluator_     = std::make_shared<BasicAlertEvaluator>(ts_);
    state_manager_ = std::make_shared<BasicAlertStateManager>(alert_repo_, event_repo_, fp_);
    scheduler_     = std::make_shared<BasicScheduler>();
    
    // 推送由 Web 层承接，此处不再启动内部 WebSocket 服务

    // 为每条规则注册调度任务
    for (const auto& r : rule_repo_->listRules()) {
        if (!r.enabled) continue;
        // 使用默认的评估间隔，因为新结构中可能没有 eval_every 字段
        const auto interval_ms = 30000; // 30秒默认间隔
        scheduler_->registerTask(r.id, interval_ms, [this, r]{
            const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            auto points = evaluator_->evaluate(r, now_ms);
            auto events = state_manager_->apply(r, points, now_ms);
            for (const auto& ev : events) {
                dispatcher_.dispatch(AlertManagerEvent::AlertEventAppended, ev);
                if (push_cb_) push_cb_(ev);
            }
        });
    }
    
    // 注册定期清理任务（每小时清理一次）
    scheduler_->registerTask("cleanup_events", 60 * 60 * 1000, [this]{
        cleanupExpiredEvents();
    });
    
    // 注册完所有任务后再启动调度器
    scheduler_->start();
    spdlog::info("AlertManager initialized with {} rules", rule_repo_->listRules().size());
}

AlertManager::~AlertManager() {
    if (scheduler_) {
        scheduler_->stop();
    }
}

// startPusher 已移除

// 规则管理（占位实现）
std::vector<Rule> AlertManager::listRules() const { return rule_repo_->listRules(); }
std::optional<Rule> AlertManager::getRule(const std::string& id) const { return rule_repo_->getRule(id); }
bool AlertManager::upsertRule(const Rule& rule) {
    scheduler_->unregisterTask(rule.id);
    const bool ok = rule_repo_->upsertRule(rule);
    if (ok && rule.enabled) {
        // 使用默认的评估间隔
        const auto interval_ms = 30000; // 30秒默认间隔
        scheduler_->registerTask(rule.id, interval_ms, [this, rule]{
            const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            auto points = evaluator_->evaluate(rule, now_ms);
            auto events = state_manager_->apply(rule, points, now_ms);
            for (const auto& ev : events) {
                dispatcher_.dispatch(AlertManagerEvent::AlertEventAppended, ev);
                if (push_cb_) push_cb_(ev);
            }
        });
    }
    return ok;
}
bool AlertManager::deleteRule(const std::string& id) {
    scheduler_->unregisterTask(id);
    return rule_repo_->deleteRule(id);
}

// 删除静默/抑制/路由/通道相关：不实现

// 告警查询与操作
std::vector<AlertEvent> AlertManager::queryEvents(const std::string& duration) const { 
    return event_repo_->query(duration); 
}

std::size_t AlertManager::countEventsByStatus(AlertStatus status) const {
    return event_repo_->countByStatus(status);
}


bool AlertManager::appendAlertEvent(const AlertEvent& event) { 
    const bool ok = event_repo_->append(event);
    if (ok) {
        dispatcher_.dispatch(AlertManagerEvent::AlertEventAppended, event);
        if (push_cb_) push_cb_(event);
    }
    return ok;
}

void AlertManager::cleanupExpiredEvents() {
    // 如果使用的是 MemoryEventRepository，执行清理
    if (auto mem_repo = std::dynamic_pointer_cast<MemoryEventRepository>(event_repo_)) {
        mem_repo->cleanup();
        spdlog::debug("Cleaned up expired events, current size: {}", mem_repo->size());
    }
}

} // namespace alert
} // namespace yw



