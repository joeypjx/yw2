#include "AlertServices.h"
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <iterator>

namespace yw {
namespace alert {

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

nlohmann::json SimpleTimeseriesProvider::evaluate(const std::string& expression,
                                                  const LabelSet& selector,
                                                  const std::string& window) {
    nlohmann::json out;
    out["rows"] = nlohmann::json::array();

    if (!conn_) return out;

    // 提取 metric 左侧（去掉比较符及右侧）
    auto expr_lhs = expression;
    for (auto op : {std::string(">="), std::string("<="), std::string("=="), std::string("!="), std::string(">"), std::string("<")}) {
        auto pos = expr_lhs.find(op);
        if (pos != std::string::npos) { expr_lhs = expr_lhs.substr(0, pos); break; }
    }
    // 去空白
    auto trim = [](std::string s){
        auto not_space = [](int ch){ return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
        return s;
    };
    expr_lhs = trim(expr_lhs);

    // 解析 metric: domain.field.agg
    std::vector<std::string> tokens;
    {
        std::stringstream ss(expr_lhs);
        std::string tok;
        while (std::getline(ss, tok, '.')) { if (!tok.empty()) tokens.push_back(tok); }
    }
    if (tokens.size() < 2) {
        return out; // 未知表达式
    }
    std::string domain = tokens[0];
    std::string field  = tokens[1];
    std::string agg    = tokens.size() >= 3 ? tokens[2] : std::string("avg");
    std::transform(agg.begin(), agg.end(), agg.begin(), ::tolower);

    auto to_pg_interval = [](const std::string& w){
        if (w.empty()) return std::string("INTERVAL '1 minutes'");
        char unit = w.back();
        std::string num = w.substr(0, w.size() - (std::isalpha(static_cast<unsigned char>(unit)) ? 1 : 0));
        if (num.empty()) num = "1";
        switch (unit) {
            case 's': return "INTERVAL '" + num + " seconds'";
            case 'h': return "INTERVAL '" + num + " hours'";
            case 'm': default: return "INTERVAL '" + num + " minutes'";
        }
    };
    const std::string interval_sql = to_pg_interval(window);

    std::string table;
    std::string value_col;
    std::vector<std::string> partition_cols;

    if (domain == "cpu") {
        table = "resource_cpu";
        value_col = field;
    } else if (domain == "memory") {
        table = "resource_memory";
        value_col = field;
    } else if (domain == "disk") {
        table = "resource_disk";
        value_col = field;
        partition_cols = {"device", "mount_point"};
    } else if (domain == "network") {
        table = "resource_network";
        value_col = field;
        partition_cols = {"interface"};
    } else if (domain == "gpu") {
        table = "resource_gpu";
        value_col = field;
        partition_cols = {"gpu_index"};
    } else {
        return out; // Unsupported domain
    }

    std::string sql;
    std::vector<std::string> where_conditions;
    std::vector<std::string> params;
    int param_count = 0;

    // Build WHERE conditions for selectors
    for (const auto& [key, value] : selector) {
        if (key == "host_ip") {
            where_conditions.push_back("host_ip = $" + std::to_string(++param_count) + "::inet");
        } else {
            where_conditions.push_back(key + " = $" + std::to_string(++param_count));
        }
        params.push_back(value);
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
    for (const auto& col : partition_cols) {
        // Cast integer gpu_index to text for consistent label handling
        if (col == "gpu_index") {
            select_cols.push_back(col + "::text AS " + col);
        } else {
            select_cols.push_back(col);
        }
        group_by_cols.push_back(col);
    }
    
    std::string select_list = join_str(select_cols, ", ");
    std::string group_by_list = join_str(group_by_cols, ", ");

    if (agg == "last" || agg == "latest") {
        sql = "SELECT DISTINCT ON (" + group_by_list + ") " + select_list + ", " + value_col + " AS value, "
              "1 AS samples, time AS last_ts FROM " + table +
              " WHERE time > now() - " + interval_sql + where_clause +
              " ORDER BY " + group_by_list + ", time DESC";
    } else {
        std::string agg_fn = "AVG";
        if (agg == "max") agg_fn = "MAX";
        else if (agg == "min") agg_fn = "MIN";
        sql = "SELECT " + select_list + ", " + agg_fn + "(" + value_col + ") AS value, "
              "COUNT(*) AS samples, MAX(time) AS last_ts FROM " + table +
              " WHERE time > now() - " + interval_sql + where_clause +
              " GROUP BY " + group_by_list;
    }

    try {
        pqxx::work tx(*conn_);
        pqxx::result r;
        
        if (!params.empty()) {
            // 动态构建参数数组
            std::vector<const char*> param_ptrs;
            for (const auto& param : params) {
                param_ptrs.push_back(param.c_str());
            }
            
            // 根据参数数量动态调用 exec_params
            switch (params.size()) {
                case 1:
                    r = tx.exec_params(sql, param_ptrs[0]);
                    break;
                case 2:
                    r = tx.exec_params(sql, param_ptrs[0], param_ptrs[1]);
                    break;
                case 3:
                    r = tx.exec_params(sql, param_ptrs[0], param_ptrs[1], param_ptrs[2]);
                    break;
                case 4:
                    r = tx.exec_params(sql, param_ptrs[0], param_ptrs[1], param_ptrs[2], param_ptrs[3]);
                    break;
                default:
                    // 如果参数太多，暂时不支持
                    return out;
            }
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
            item["samples"] = row["samples"].as<long long>(0);
            item["last_ts"] = row["last_ts"].as<std::string>("");
            arr.push_back(std::move(item));
        }
    } catch (...) {
        // 返回空 rows 即可
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
bool MemoryEventRepository::append(const AlertEvent& event) {
    std::lock_guard<std::mutex> lk(mu_);
    events_.push_back(event);
    std::cout << "append event: " << event.fingerprint << " " << event.rule_id  << " " << event.value << " " << event.context << " " << event.action << std::endl;
    for (const auto& e : events_) {
        std::cout << "event: " << e.fingerprint << " " << e.rule_id  << " " << e.value << " " << e.context << " " << e.action << std::endl;
    }
    return true;
}

std::vector<AlertEvent> MemoryEventRepository::query(const std::string& /*duration*/) const {
    std::lock_guard<std::mutex> lk(mu_);
    return events_;
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

std::vector<EvaluationPoint> BasicAlertEvaluator::evaluate(const Rule& rule, std::int64_t /*now_ms*/) {
    std::vector<EvaluationPoint> out;

    // 解析表达式：支持单一比较与区间（使用 && 拼接两个比较）
    const auto pcs = parse_conditions(rule.expression);

    // 调用时序提供者：约定返回 JSON 对象，包含 rows 数组
    nlohmann::json res = ts_ ? ts_->evaluate(rule.expression, rule.selector, rule.window) : nlohmann::json();
    if (!res.is_object() || !res.contains("rows") || !res["rows"].is_array()) {
        return out; // 无数据
    }

    for (const auto& row : res["rows"]) {
        EvaluationPoint p;
        // labels
        if (row.contains("labels") && row["labels"].is_object()) {
            for (auto it = row["labels"].begin(); it != row["labels"].end(); ++it) {
                if (it.value().is_string()) {
                    p.labels[it.key()] = it.value().get<std::string>();
                } else if (it.value().is_number()) {
                    p.labels[it.key()] = it.value().dump();
                } else {
                    p.labels[it.key()] = it.value().dump();
                }
            }
        }

        // value
        double v = 0.0;
        if (row.contains("value") && (row["value"].is_number_float() || row["value"].is_number_integer())) {
            v = row["value"].get<double>();
        }
        p.value = v;

        // matched：优先使用 provider 结果，否则本地依据阈值/区间计算
        if (row.contains("matched") && row["matched"].is_boolean()) {
            p.matched = row["matched"].get<bool>();
        } else if (pcs.ok) {
            bool all_ok = true;
            for (const auto& c : pcs.conds) {
                if (!eval_match(v, c.op, c.threshold)) { all_ok = false; break; }
            }
            p.matched = all_ok;
        } else {
            p.matched = false;
        }

        // context：合并窗口、metric 与 provider 返回的上下文
        p.context["window"] = rule.window;
        p.context["metric"] = rule.expression;
        if (row.contains("context")) {
            p.context["detail"] = row["context"]; // 嵌入 provider 明细
        }
        if (row.contains("samples")) {
            p.context["samples"] = row["samples"];
        }
        if (row.contains("last_ts")) {
            p.context["last_ts"] = row["last_ts"];
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
        auto st = repo_->getState(fingerprint).value_or(AlertState{ fingerprint, rule.id });
        st.rule_id = rule.id;
        st.severity = rule.severity;
        st.labels = pt.labels;
        st.last_eval_ms = now_ms;

        AlertEvent ev;
        ev.timestamp_ms = now_ms;
        ev.fingerprint = fingerprint;
        ev.rule_id = rule.id;
        ev.severity = rule.severity;
        ev.labels = pt.labels;
        ev.title = rule.name;           // 从规则名称获取标题
        ev.description = rule.description; // 从规则描述获取描述
        ev.value = pt.value;
        ev.context = pt.context;

        if (pt.matched) {
            // 增加命中计数
            st.occurrences++;
            
            if (st.status == AlertStatus::Inactive || st.status == AlertStatus::Resolved) {
                // 首次命中，进入 Pending 状态
                st.status = AlertStatus::Pending;
                st.first_firing_ms = now_ms;
                st.last_change_ms = now_ms;
                ev.action = "pending";
                ev.status = AlertStatus::Pending;
                out.push_back(ev);
                event_repo_->append(ev);
            } else if (st.status == AlertStatus::Pending && st.occurrences >= rule.for_times) {
                // 达到 for_times 阈值，进入 Firing 状态
                st.status = AlertStatus::Firing;
                st.last_change_ms = now_ms;
                ev.action = "firing";
                ev.status = AlertStatus::Firing;
                out.push_back(ev);
                event_repo_->append(ev);
            }
            // 如果已经是 Firing 状态，继续更新计数但不产生新事件
        } else {
            // 不匹配，重置计数
            st.occurrences = 0;
            
            if (st.status == AlertStatus::Firing || st.status == AlertStatus::Pending) {
                // 从 Firing/Pending 状态恢复
                st.status = AlertStatus::Resolved;
                st.last_change_ms = now_ms;
                ev.action = "resolved";
                ev.status = AlertStatus::Resolved;
                out.push_back(ev);
                event_repo_->append(ev);
            }
        }

        repo_->upsertState(st);
    }
    return out;
}

std::vector<AlertState> BasicAlertStateManager::listActive(const LabelSet& matcher) const {
    return repo_->listActive(matcher);
}

bool BasicAlertStateManager::ack(const std::string& fingerprint, const std::string& user, const std::string& comment) {
    auto st_opt = repo_->getState(fingerprint);
    if (!st_opt) return false;
    auto st = *st_opt;
    st.acked = true;
    st.acked_by = user;
    st.acked_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return repo_->upsertState(st);
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
                        std::cout << "runTask: " << id << " " << now_ms << " " << t.next_run_ms << std::endl;
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
    std::cout << "registerTask: " << id << " " << interval_ms << " " << now_ms + interval_ms << std::endl;
    std::cout << "tasks_.size(): " << tasks_.size() << std::endl;
}

void BasicScheduler::unregisterTask(const std::string& id) {
    std::lock_guard<std::mutex> lk(mu_);
    tasks_.erase(id);
}

} // namespace alert
} // namespace yw


