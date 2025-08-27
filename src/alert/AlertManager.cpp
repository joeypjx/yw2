// 1. 先包含 AlertManager.h
#include "AlertManager.h"
#include "DatabaseEventRepository.h"
#include "DatabaseRuleRepository.h"
#include <chrono>
#include <cctype>

namespace yw {
namespace alert {

namespace {
static std::int64_t parseDurationMs(const std::string& s, std::int64_t default_ms) {
    if (s.empty()) return default_ms;
    // 仅支持 123s/123m/123h
    char unit = s.back();
    std::size_t len = s.size();
    if (std::isalpha(static_cast<unsigned char>(unit))) {
        len -= 1;
    } else {
        unit = 's';
    }
    std::int64_t val = 0;
    try {
        val = std::stoll(s.substr(0, len));
    } catch (...) {
        return default_ms;
    }
    switch (unit) {
        case 's': return val * 1000;
        case 'm': return val * 60 * 1000;
        case 'h': return val * 60 * 60 * 1000;
        default:  return default_ms;
    }
}
}

AlertManager::AlertManager() {
    // 创建数据库连接
    try {
        conn_ = std::make_shared<pqxx::connection>("postgres://postgres:HZ715Net@localhost:5432/yw");
    } catch (const std::exception& e) {
        // 如果数据库连接失败，conn_ 将为 nullptr
        // 后续服务初始化时会检查 conn_ 状态
    }
    
    // 组装最小运行所需服务（数据库实现 + 简易提供者）
    rule_repo_     = std::make_shared<DatabaseRuleRepository>(conn_);
    alert_repo_    = std::make_shared<MemoryAlertRepository>();
    event_repo_    = std::make_shared<DatabaseEventRepository>(conn_);
    fp_            = std::make_shared<SimpleFingerprintGenerator>();
    ts_            = std::make_shared<SimpleTimeseriesProvider>(conn_);
    evaluator_     = std::make_shared<BasicAlertEvaluator>(ts_);
    state_manager_ = std::make_shared<BasicAlertStateManager>(alert_repo_, event_repo_, fp_);
    scheduler_     = std::make_shared<BasicScheduler>();
    
    // 启动内部 WebSocket 推送（默认端口 18999）
    (void)pusher_.start(":8081");

    // 为每条规则注册调度任务
    scheduler_->start();

    for (const auto& r : rule_repo_->listRules()) {
        if (!r.enabled) continue;
        const auto interval_ms = parseDurationMs(r.eval_every, 30000);
        scheduler_->registerTask(r.id, interval_ms, [this, r]{
            const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            auto points = evaluator_->evaluate(r, now_ms);
            auto events = state_manager_->apply(r, points, now_ms);
            for (const auto& ev : events) {
                dispatcher_.dispatch(AlertManagerEvent::AlertEventAppended, ev);
                pusher_.push(ev);
            }
        });
    }
}

AlertManager::~AlertManager() = default;

bool AlertManager::startPusher(const std::string& ip_port) {
    return pusher_.start(ip_port.empty() ? ":8081" : ip_port.c_str());
}

// 规则管理（占位实现）
std::vector<Rule> AlertManager::listRules() const { return rule_repo_->listRules(); }
std::optional<Rule> AlertManager::getRule(const std::string& id) const { return rule_repo_->getRule(id); }
bool AlertManager::upsertRule(const Rule& rule) {
    scheduler_->unregisterTask(rule.id);
    const bool ok = rule_repo_->upsertRule(rule);
    if (ok && rule.enabled) {
        const auto interval_ms = parseDurationMs(rule.eval_every, 30000);
        scheduler_->registerTask(rule.id, interval_ms, [this, rule]{
            const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            auto points = evaluator_->evaluate(rule, now_ms);
            auto events = state_manager_->apply(rule, points, now_ms);
            for (const auto& ev : events) {
                dispatcher_.dispatch(AlertManagerEvent::AlertEventAppended, ev);
                pusher_.push(ev);
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

// 告警查询与操作（占位实现）
std::vector<AlertEvent> AlertManager::queryEvents(const std::string& duration) const { return event_repo_->query(duration); }
bool AlertManager::ackAlert(const std::string& fingerprint, const std::string& user, const std::string& comment) { return state_manager_->ack(fingerprint, user, comment); }
bool AlertManager::appendAlertEvent(const AlertEvent& event) { return event_repo_->append(event); }

} // namespace alert
} // namespace yw



