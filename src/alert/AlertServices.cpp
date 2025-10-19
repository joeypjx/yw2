#include "AlertServices.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <iterator>
#include "yw/DurationUtils.h"

namespace yw {
namespace alert {

namespace {
// 评估单个条件
static bool eval_condition(double value, const Condition& cond) {
    const auto& op = cond.op;
    const auto& th = cond.threshold;
    if (op == ">=") return value >= th;
    if (op == "<=") return value <= th;
    if (op == "==") return value == th;
    if (op == "!=") return value != th;
    if (op == ">")  return value >  th;
    if (op == "<")  return value <  th;
    return false;
}

// 检查标签是否匹配
static bool match_tags(const LabelSet& labels, const LabelSet& tag_filter) {
    for (const auto& [key, value] : tag_filter) {
        auto it = labels.find(key);
        if (it == labels.end() || it->second != value) {
            return false; // 标签不匹配
        }
    }
    return true; // 所有标签都匹配
}
}

// SimpleFingerprintGenerator
std::string SimpleFingerprintGenerator::generate(const std::string& rule_id, const LabelSet& labels) const {
    // 简易拼接（可替换为稳定哈希）
    std::string s = rule_id;
    for (const auto& [k, v] : labels) {
        s += "|" + k + "=" + v;
    }
    return s;
}

// SimpleTimeseriesProvider
SimpleTimeseriesProvider::SimpleTimeseriesProvider(std::shared_ptr<pqxx::connection> conn)
    : conn_(std::move(conn)) {}

nlohmann::json SimpleTimeseriesProvider::evaluate(const Expression& expr,
                                                   const std::string& window,
                                                   std::int64_t /*now_ms*/) {
    nlohmann::json out;
    out["rows"] = nlohmann::json::array();

    if (!conn_) return out;

    const std::string& stable = expr.stable;
    const std::string& metric = expr.metric;

    const std::string interval_sql = yw::utils::DurationUtils::parseToPgInterval(window);

    std::string table;
    std::string value_col = metric;
    std::vector<std::string> partition_cols;

    // 根据 stable 确定表名
    if (stable == "cpu") {
        table = "resource_cpu";
    } else if (stable == "memory") {
        table = "resource_memory";
    } else if (stable == "disk") {
        table = "resource_disk";
        partition_cols = {"device", "mount_point"};
    } else if (stable == "network") {
        table = "resource_network";
        partition_cols = {"interface"};
    } else if (stable == "gpu") {
        table = "resource_gpu";
        partition_cols = {"gpu_index"};
    } else if (stable == "alive") {
        table = "resource_alive";
        value_col = "alive";
    } else {
        return out; // Unsupported stable
    }

    // 构建 WHERE 条件（基于 tags）
    std::vector<std::string> where_conditions;
    std::vector<std::string> params;
    int param_count = 0;

    // 如果有 tags 过滤，构建 WHERE 条件
    // 注意：多个 tags 集合是 OR 关系，但这里简化为查询所有匹配的数据
    // 实际过滤在评估器中进行
    if (!expr.tags.empty()) {
        // 提取所有可能的 host_ip
        std::vector<std::string> host_ips;
        for (const auto& tag_set : expr.tags) {
            auto it = tag_set.find("host_ip");
            if (it != tag_set.end()) {
                host_ips.push_back(it->second);
            }
        }

        if (!host_ips.empty() && host_ips.size() == 1) {
            // 只有一个 host_ip
            where_conditions.push_back("host_ip = $" + std::to_string(++param_count) + "::inet");
            params.push_back(host_ips[0]);
        }
        // 如果有多个 host_ip 或其他复杂条件，暂时查询所有数据，在内存中过滤
    }

    auto join_str = [](const std::vector<std::string>& vec, const std::string& delim) {
        if (vec.empty()) return std::string();
        return std::accumulate(std::next(vec.begin()), vec.end(), vec[0],
            [&delim](const std::string& a, const std::string& b) {
                return a + delim + b;
            });
    };

    std::string where_clause;
    if (!where_conditions.empty()) {
        where_clause = " AND " + join_str(where_conditions, " AND ");
    }

    std::vector<std::string> select_cols = {"host_ip::text AS host_ip"};
    std::vector<std::string> group_by_cols = {"host_ip"};
    std::vector<std::string> order_by_cols = {"host_ip", "time DESC"};

    for (const auto& col : partition_cols) {
        if (col == "gpu_index") {
            select_cols.push_back(col + "::text AS " + col);
        } else {
            select_cols.push_back(col);
        }
        group_by_cols.push_back(col);
        order_by_cols.insert(order_by_cols.begin() + order_by_cols.size() - 1, col);
    }

    std::string select_list = join_str(select_cols, ", ");
    std::string group_by_list = join_str(group_by_cols, ", ");
    std::string order_by_list = join_str(order_by_cols, ", ");

    std::string sql;

    if (stable == "alive") {
        // 特殊处理：心跳检测
        if (!expr.tags.empty() && expr.tags[0].count("host_ip")) {
            sql = "SELECT host(h.host_ip) AS host_ip, "
                  "COALESCE(MAX(a.alive), 0) AS value, "
                  "COALESCE(COUNT(a.alive), 0) AS samples, MAX(a.time) AS last_ts "
                  "FROM (SELECT $1::inet AS host_ip) h "
                  "LEFT JOIN resource_alive a ON a.host_ip = h.host_ip AND a.time > now() - " + interval_sql + " "
                  "GROUP BY h.host_ip";
        } else {
            sql = "WITH dims AS (SELECT DISTINCT host_ip FROM resource_alive) "
                  "SELECT host(d.host_ip) AS host_ip, "
                  "COALESCE(MAX(a.alive), 0) AS value, "
                  "COALESCE(COUNT(a.alive), 0) AS samples, MAX(a.time) AS last_ts "
                  "FROM dims d "
                  "LEFT JOIN resource_alive a ON a.host_ip = d.host_ip AND a.time > now() - " + interval_sql + " "
                  "GROUP BY d.host_ip";
        }
    } else {
        // 查询时间窗口内的所有数据点（不聚合，用于检查持续时间）
        sql = "SELECT " + select_list + ", " + value_col + " AS value, time "
              "FROM " + table +
              " WHERE time > now() - " + interval_sql + where_clause +
              " ORDER BY " + order_by_list;
    }

    try {
        pqxx::work tx(*conn_);
        pqxx::result r;

        if (!params.empty()) {
            r = tx.exec_params(sql, params[0]);
        } else {
            r = tx.exec(sql);
        }

        tx.commit();
        auto& arr = out["rows"];
        for (const auto& row : r) {
            nlohmann::json item;
            nlohmann::json labels;
            labels["host_ip"] = row["host_ip"].as<std::string>();
            for (const auto& col : partition_cols) {
                if (!row[col].is_null()) {
                    labels[col] = row[col].as<std::string>();
                }
            }
            item["labels"] = std::move(labels);
            item["value"] = row["value"].is_null() ? 0.0 : row["value"].as<double>();

            if (row.column_number("time") >= 0 && !row["time"].is_null()) {
                item["time"] = row["time"].as<std::string>();
            }
            if (row.column_number("samples") >= 0) {
                item["samples"] = row["samples"].as<long long>(1);
            }
            if (row.column_number("last_ts") >= 0 && !row["last_ts"].is_null()) {
                item["last_ts"] = row["last_ts"].as<std::string>();
            }

            arr.push_back(std::move(item));
        }
    } catch (const std::exception& e) {
        // 记录错误但返回空结果
        out["error"] = e.what();
    }

    return out;
}

// MemoryAlertRepository
std::optional<AlertState> MemoryAlertRepository::getState(const std::string& fingerprint) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = states_.find(fingerprint);
    if (it == states_.end()) return std::nullopt;
    return it->second;
}

bool MemoryAlertRepository::upsertState(const AlertState& state) {
    std::lock_guard<std::mutex> lk(mu_);
    states_[state.fingerprint] = state;
    return true;
}

std::vector<AlertState> MemoryAlertRepository::listActive(const LabelSet& /*matcher*/) const {
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<AlertState> v;
    v.reserve(states_.size());
    for (const auto& [fp, st] : states_) {
        if (st.status == AlertStatus::Firing || st.status == AlertStatus::Pending) v.push_back(st);
    }
    return v;
}

// MemoryEventRepository
MemoryEventRepository::MemoryEventRepository(std::size_t max_events, std::int64_t max_age_ms)
    : max_events_(max_events), max_age_ms_(max_age_ms) {
    events_.reserve(max_events_); // 预分配空间
}

bool MemoryEventRepository::append(const AlertEvent& event) {
    std::lock_guard<std::mutex> lk(mu_);
    
    // 如果超过最大容量，先清理过期事件
    if (events_.size() >= max_events_) {
        cleanupInternal();
    }
    
    // 如果清理后仍然超过容量，删除最旧的事件
    if (events_.size() >= max_events_) {
        events_.erase(events_.begin(), events_.begin() + (events_.size() - max_events_ + 1));
    }
    
    events_.push_back(event);
    return true;
}

std::vector<AlertEvent> MemoryEventRepository::query(const std::string& /*duration*/) const {
    std::lock_guard<std::mutex> lk(mu_);
    return events_;
}

std::size_t MemoryEventRepository::countByStatus(AlertStatus status) const {
    std::lock_guard<std::mutex> lk(mu_);
    return static_cast<std::size_t>(std::count_if(events_.begin(), events_.end(), [status](const AlertEvent& e){
        // 将 AlertStatus 枚举转换为字符串进行比较
        std::string status_str;
        switch (status) {
            case AlertStatus::Inactive: status_str = "inactive"; break;
            case AlertStatus::Pending: status_str = "pending"; break;
            case AlertStatus::Firing: status_str = "firing"; break;
            case AlertStatus::Resolved: status_str = "resolved"; break;
        }
        return e.status == status_str;
    }));
}

void MemoryEventRepository::cleanup() {
    std::lock_guard<std::mutex> lk(mu_);
    cleanupInternal();
}

std::size_t MemoryEventRepository::size() const {
    std::lock_guard<std::mutex> lk(mu_);
    return events_.size();
}

bool MemoryEventRepository::hasEvent(const std::string& fingerprint) const {
    std::lock_guard<std::mutex> lk(mu_);
    return std::any_of(events_.begin(), events_.end(), [&fingerprint](const AlertEvent& event) {
        return event.fingerprint == fingerprint;
    });
}

std::optional<AlertEvent> MemoryEventRepository::getEvent(const std::string& fingerprint) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = std::find_if(events_.begin(), events_.end(), [&fingerprint](const AlertEvent& event) {
        return event.fingerprint == fingerprint;
    });
    if (it != events_.end()) {
        return *it;
    }
    return std::nullopt;
}

bool MemoryEventRepository::updateEvent(const AlertEvent& event) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = std::find_if(events_.begin(), events_.end(), [&event](const AlertEvent& e) {
        return e.fingerprint == event.fingerprint;
    });
    if (it != events_.end()) {
        *it = event;
        return true;
    }
    return false;
}

void MemoryEventRepository::cleanupInternal() {
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // 删除过期事件
    events_.erase(
        std::remove_if(events_.begin(), events_.end(), [this, now_ms](const AlertEvent& event) {
            // 尝试解析时间戳
            try {
                if (!event.created_at.empty()) {
                    // 假设 created_at 是毫秒时间戳字符串
                    std::int64_t created_ms = std::stoll(event.created_at);
                    return (now_ms - created_ms) > max_age_ms_;
                }
                return false; // 如果没有时间戳，保留事件
            } catch (...) {
                return false; // 解析失败，保留事件
            }
        }),
        events_.end()
    );
}

// MemoryRuleRepository
std::vector<Rule> MemoryRuleRepository::listRules() const {
    std::lock_guard<std::mutex> lk(mu_);
    std::vector<Rule> v; v.reserve(rules_.size());
    for (const auto& [id, r] : rules_) v.push_back(r);
    return v;
}

std::optional<Rule> MemoryRuleRepository::getRule(const std::string& id) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = rules_.find(id);
    if (it == rules_.end()) return std::nullopt;
    return it->second;
}

bool MemoryRuleRepository::upsertRule(const Rule& rule) {
    std::lock_guard<std::mutex> lk(mu_);
    rules_[rule.id] = rule;
    return true;
}

bool MemoryRuleRepository::deleteRule(const std::string& id) {
    std::lock_guard<std::mutex> lk(mu_);
    return rules_.erase(id) > 0;
}

// BasicAlertEvaluator
BasicAlertEvaluator::BasicAlertEvaluator(std::shared_ptr<ITimeseriesProvider> ts)
    : ts_(std::move(ts)) {}

namespace {
struct ParsedCond {
    std::string op;
    double      threshold = 0.0;
};

struct ParsedConds {
    std::vector<ParsedCond> conds;
    bool ok = false;
};

static ParsedCond parse_single_condition(const std::string& expr, bool& ok) {
    // 支持的操作符顺序很重要：>=, <=, ==, !=, >, <
    static const std::vector<std::string> ops = {">=","<=","==","!=",">","<"};
    for (const auto& o : ops) {
        auto pos = expr.find(o);
        if (pos != std::string::npos) {
            // 提取阈值
            std::size_t start = pos + o.size();
            while (start < expr.size() && std::isspace(static_cast<unsigned char>(expr[start]))) start++;
            std::size_t end = start;
            bool dot_seen = false;
            while (end < expr.size()) {
                char c = expr[end];
                if ((c >= '0' && c <= '9') || c == '-' || (!dot_seen && c == '.')) {
                    if (c == '.') dot_seen = true;
                    end++;
                } else break;
            }
            try {
                double th = std::stod(expr.substr(start, end - start));
                ok = true;
                return ParsedCond{o, th};
            } catch (...) {
                ok = false;
                return ParsedCond{"", 0.0};
            }
        }
    }
    ok = false;
    return ParsedCond{"", 0.0};
}

static ParsedConds parse_conditions(const std::string& expr) {
    ParsedConds out;
    // 支持区间：使用 "&&" 连接两个比较，例如：metric >= 10 && metric <= 20
    // 也兼容单一比较：metric > 10
    std::vector<std::string> parts;
    {
        std::string s = expr;
        // 简单分割 "&&"
        std::size_t start = 0;
        while (true) {
            auto pos = s.find("&&", start);
            if (pos == std::string::npos) {
                parts.emplace_back(s.substr(start));
                break;
            }
            parts.emplace_back(s.substr(start, pos - start));
            start = pos + 2;
        }
    }
    for (auto& p : parts) {
        // 去空白
        auto l = p.find_first_not_of(" \t\n\r");
        auto r = p.find_last_not_of(" \t\n\r");
        if (l == std::string::npos) continue;
        std::string trimmed = p.substr(l, r - l + 1);
        bool ok_single = false;
        auto cond = parse_single_condition(trimmed, ok_single);
        if (!ok_single) { out.ok = false; out.conds.clear(); return out; }
        out.conds.push_back(cond);
    }
    out.ok = !out.conds.empty();
    return out;
}

static bool eval_match(double v, const std::string& op, double th) {
    if (op == ">=") return v >= th;
    if (op == "<=") return v <= th;
    if (op == "==") return v == th;
    if (op == "!=") return v != th;
    if (op == ">")  return v >  th;
    if (op == "<")  return v <  th;
    return false;
}
}

std::vector<EvaluationPoint> BasicAlertEvaluator::evaluate(const Rule& rule, std::int64_t now_ms) {
    std::vector<EvaluationPoint> out;

    const auto& expr = rule.expression;

    // 调用时序提供者获取数据
    nlohmann::json res = ts_ ? ts_->evaluate(expr, rule.for_duration, now_ms) : nlohmann::json();
    if (!res.is_object() || !res.contains("rows") || !res["rows"].is_array()) {
        return out; // 无数据
    }

    // 按标签分组数据点（用于检查持续时间）
    std::unordered_map<std::string, std::vector<nlohmann::json>> grouped_data;

    for (const auto& row : res["rows"]) {
        if (!row.contains("labels") || !row["labels"].is_object()) {
            continue;
        }

        // 提取标签
        LabelSet labels;
        for (auto it = row["labels"].begin(); it != row["labels"].end(); ++it) {
            if (it.value().is_string()) {
                labels[it.key()] = it.value().get<std::string>();
            } else {
                labels[it.key()] = it.value().dump();
            }
        }

        // 检查 tags 过滤条件
        bool tag_matched = false;
        if (expr.tags.empty()) {
            // 没有指定 tags，匹配所有
            tag_matched = true;
        } else {
            // 只要匹配任意一个 tag 集合即可
            for (const auto& tag_filter : expr.tags) {
                if (match_tags(labels, tag_filter)) {
                    tag_matched = true;
                    break;
                }
            }
        }

        if (!tag_matched) {
            continue; // 不符合 tags 过滤条件，跳过
        }

        // 生成标签指纹（用于分组）
        std::string fingerprint;
        for (const auto& [k, v] : labels) {
            fingerprint += k + "=" + v + "|";
        }

        grouped_data[fingerprint].push_back(row);
    }

    // 对每个分组检查是否所有数据点都满足条件
    for (const auto& [fingerprint, rows] : grouped_data) {
        if (rows.empty()) continue;

        // 检查该分组的所有数据点是否都满足条件
        bool all_matched = true;
        double latest_value = 0.0;
        LabelSet labels;

        for (const auto& row : rows) {
            // 提取值
            double value = 0.0;
            if (row.contains("value") && (row["value"].is_number_float() || row["value"].is_number_integer())) {
                value = row["value"].get<double>();
            }
            latest_value = value; // 记录最新值

            // 检查所有条件
            bool row_matched = true;
            for (const auto& cond : expr.conditions) {
                if (!eval_condition(value, cond)) {
                    row_matched = false;
                    break;
                }
            }

            if (!row_matched) {
                all_matched = false;
                break; // 有一个点不满足，整个分组就不满足
            }

            // 提取标签（从第一个 row）
            if (labels.empty() && row.contains("labels") && row["labels"].is_object()) {
                for (auto it = row["labels"].begin(); it != row["labels"].end(); ++it) {
                    if (it.value().is_string()) {
                        labels[it.key()] = it.value().get<std::string>();
                    } else {
                        labels[it.key()] = it.value().dump();
                    }
                }
            }
        }

        // 构建评估点
        EvaluationPoint p;
        p.labels = labels;
        p.value = latest_value;
        p.matched = all_matched;

        // 构建上下文信息
        p.context["rule_id"] = rule.id;
        p.context["alert_name"] = rule.alert_name;
        p.context["summary"] = rule.summary;
        p.context["description"] = rule.description;
        p.context["alert_type"] = rule.alert_type;
        p.context["for_duration"] = rule.for_duration;
        p.context["stable"] = expr.stable;
        p.context["metric"] = expr.metric;
        p.context["sample_count"] = rows.size();

        // 严重级别
        std::string severity_str;
        switch (rule.severity) {
            case Severity::Info: severity_str = "提示"; break;
            case Severity::Critical: severity_str = "严重"; break;
            case Severity::Warn: default: severity_str = "一般"; break;
        }
        p.context["severity"] = severity_str;

        // 将标签并入 context（用于模板替换）
        for (const auto& [key, value] : labels) {
            p.context[key] = value;
        }

        out.push_back(std::move(p));
    }

    return out;
}

// BasicAlertStateManager
BasicAlertStateManager::BasicAlertStateManager(std::shared_ptr<IAlertRepository> repo,
                                               std::shared_ptr<IEventRepository> event_repo,
                                               std::shared_ptr<IFingerprintGenerator> fp)
    : repo_(std::move(repo)), event_repo_(std::move(event_repo)), fp_(std::move(fp)) {}

std::vector<AlertEvent> BasicAlertStateManager::apply(const Rule& rule,
                                                      const std::vector<EvaluationPoint>& points,
                                                      std::int64_t now_ms) {
    std::vector<AlertEvent> out;
    for (const auto& pt : points) {
        auto fingerprint = fp_->generate(rule.id, pt.labels);
        auto st = repo_->getState(fingerprint).value_or(AlertState{ fingerprint });
        st.labels = pt.labels;

        // 检查是否已存在事件，且事件状态不是 resolved
        bool event_exists = event_repo_->hasEvent(fingerprint);
        bool should_update_existing = false;
        AlertEvent ev;
        
        if (event_exists) {
            // 获取现有事件
            ev = event_repo_->getEvent(fingerprint).value();
            // 只有当事件状态不是 resolved 时才更新现有事件
            should_update_existing = (ev.status != "resolved");
        }
        
        if (!should_update_existing) {
            // 创建新事件（首次创建或重新触发）
            ev.fingerprint = fingerprint;
            ev.labels = pt.labels;
            ev.summary = rule.summary;
            ev.description = rule.description;
            ev.created_at = std::to_string(now_ms);
            ev.status = ""; // 清空状态，后续会设置
        }

        bool status_changed = false;
        std::string old_status = ev.status;

        if (pt.matched) {
            if (st.status == AlertStatus::Inactive || st.status == AlertStatus::Resolved) {
                // 首次命中，进入 Pending 状态
                st.status = AlertStatus::Pending;
                ev.status = "pending";
                ev.starts_at = std::to_string(now_ms);
                status_changed = true;
            } else if (st.status == AlertStatus::Pending) {
                // 达到阈值，进入 Firing 状态（简化：直接进入 firing）
                st.status = AlertStatus::Firing;
                ev.status = "firing";
                status_changed = true;
            }
            // 如果已经是 Firing 状态，继续更新计数但不改变状态
        } else {
            if (st.status == AlertStatus::Firing || st.status == AlertStatus::Pending) {
                // 从 Firing/Pending 状态恢复
                st.status = AlertStatus::Resolved;
                ev.status = "resolved";
                ev.ends_at = std::to_string(now_ms);
                status_changed = true;
            }
        }

        // 更新事件时间戳
        ev.updated_at = std::to_string(now_ms);

        // 只有状态变化时才处理事件
        if (status_changed) {
            if (should_update_existing) {
                // 更新现有事件
                event_repo_->updateEvent(ev);
            } else {
                // 创建新事件
                event_repo_->append(ev);
            }
            out.push_back(ev);
        }

        repo_->upsertState(st);
    }
    return out;
}

std::vector<AlertState> BasicAlertStateManager::listActive(const LabelSet& matcher) const {
    return repo_->listActive(matcher);
}


// BasicScheduler（单线程轮询）
BasicScheduler::BasicScheduler() = default;
BasicScheduler::~BasicScheduler() { stop(); }

void BasicScheduler::start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread([this]{
        while (running_.load()) {
            const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            {
                std::lock_guard<std::mutex> lk(mu_);
                for (auto& [id, t] : tasks_) {
                    if (now_ms >= t.next_run_ms) {
                        try { t.task(); } catch (...) {}
                        t.next_run_ms = now_ms + t.interval_ms;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void BasicScheduler::stop() {
    if (!running_.exchange(false)) return;
    if (worker_.joinable()) worker_.join();
}

void BasicScheduler::registerTask(const std::string& id, std::int64_t interval_ms, Task task) {
    std::lock_guard<std::mutex> lk(mu_);
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    tasks_[id] = TaskEntry{interval_ms, std::move(task), now_ms + interval_ms};
}

void BasicScheduler::unregisterTask(const std::string& id) {
    std::lock_guard<std::mutex> lk(mu_);
    tasks_.erase(id);
}

} // namespace alert
} // namespace yw



