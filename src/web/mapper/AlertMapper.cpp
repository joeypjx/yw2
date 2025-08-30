#include "AlertMapper.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace yw {
namespace web {
namespace mapper {

static std::string trimStr(std::string s){
    auto not_space = [](int ch){ return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

// 将模板字符串中的 {{key}} 用 labels[key] 替换
static std::string renderDescription(const std::string& tmpl, const alert::LabelSet& labels) {
    std::string out;
    out.reserve(tmpl.size());
    size_t i = 0;
    while (i < tmpl.size()) {
        size_t l = tmpl.find("{{", i);
        if (l == std::string::npos) {
            out.append(tmpl, i, std::string::npos);
            break;
        }
        out.append(tmpl, i, l - i);
        size_t r = tmpl.find("}}", l + 2);
        if (r == std::string::npos) {
            out.append(tmpl, l, std::string::npos);
            break;
        }
        std::string key = tmpl.substr(l + 2, r - (l + 2));
        key = trimStr(key);
        auto it = labels.find(key);
        if (it != labels.end()) out.append(it->second);
        else out.append(tmpl, l, r - l + 2);
        i = r + 2;
    }
    return out;
}

std::string formatTimestampMs(std::int64_t ms) {
    if (ms <= 0) return std::string();
    std::time_t t = static_cast<std::time_t>(ms / 1000);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

long long parseDurationSeconds(const std::string& sIn) {
    std::string s = trimStr(sIn);
    if (s.empty()) return 0;
    char unit = s.back();
    bool has_unit = std::isalpha(static_cast<unsigned char>(unit)) != 0;
    std::string num = has_unit ? s.substr(0, s.size()-1) : s;
    num = trimStr(num);
    if (num.empty()) return 0;
    long long n = 0; try { n = std::stoll(num); } catch (...) { return 0; }
    if (!has_unit) return n;
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(unit)))) {
        case 's': return n;
        case 'm': return n * 60;
        case 'h': return n * 3600;
        default: return n;
    }
}

std::string formatSecondsCompact(long long sec) {
    if (sec <= 0) return std::string("0s");
    if (sec % 3600 == 0) return std::to_string(sec/3600) + "h";
    if (sec % 60 == 0) return std::to_string(sec/60) + "m";
    return std::to_string(sec) + "s";
}

nlohmann::json parseExpressionObject(const std::string& expr) {
    nlohmann::json obj;
    obj["stable"] = "";
    obj["metric"] = "";
    obj["conditions"] = nlohmann::json::array();
    obj["tags"] = nlohmann::json::array();
    if (expr.empty()) return obj;

    auto split_conditions = [](const std::string& e){
        std::vector<std::string> parts; std::string cur;
        for (size_t i = 0; i < e.size(); ++i) {
            if (i + 1 < e.size() && e[i]=='&' && e[i+1]=='&') { parts.push_back(trimStr(cur)); cur.clear(); ++i; }
            else cur.push_back(e[i]);
        }
        if (!cur.empty()) parts.push_back(trimStr(cur));
        return parts;
    };
    auto conds = split_conditions(expr);

    for (const auto& c : conds) {
        static const std::vector<std::string> ops = {">=","<=","==","!=",">","<"};
        size_t pos = std::string::npos; std::string op;
        for (const auto& o : ops) { auto p = c.find(o); if (p != std::string::npos) { pos=p; op=o; break; } }
        if (pos == std::string::npos) continue;
        std::string lhs = trimStr(c.substr(0, pos));
        std::string rhs = trimStr(c.substr(pos + op.size()));
        std::vector<std::string> tokens; tokens.reserve(3);
        {
            std::string buf;
            for (char ch : lhs) {
                if (ch == '.') { if (!buf.empty()) { tokens.push_back(buf); buf.clear(); } }
                else buf.push_back(ch);
            }
            if (!buf.empty()) tokens.push_back(buf);
        }
        std::string stable = tokens.size() >= 1 ? tokens[0] : std::string();
        std::string metric = tokens.size() >= 2 ? tokens[1] : (tokens.empty() ? std::string() : tokens.back());
        if (obj["stable"].get<std::string>().empty()) obj["stable"] = stable;
        if (obj["metric"].get<std::string>().empty()) obj["metric"] = metric;
        double th = 0.0; try { th = std::stod(rhs); } catch (...) { th = 0.0; }
        obj["conditions"].push_back({ {"operator", op}, {"threshold", th} });
    }
    return obj;
}

std::string buildExpressionString(const nlohmann::json& exprObj) {
    std::string domain = exprObj.value("stable", std::string());
    std::string metric = exprObj.value("metric", std::string());
    std::string lhs = domain; if (!lhs.empty()) lhs += "."; lhs += metric;
    std::string out;
    if (exprObj.contains("conditions") && exprObj["conditions"].is_array()) {
        bool first = true;
        for (const auto& c : exprObj["conditions"]) {
            if (!c.is_object()) continue;
            std::string op = c.value("operator", "");
            if (op != ">" && op != "<" && op != "==" && op != "!=" && op != "<=" && op != ">=") continue;
            double th = c.value("threshold", 0.0);
            if (!first) out += " && ";
            out += lhs + " " + op + " " + std::to_string(th);
            first = false;
        }
    }
    if (out.empty()) out = lhs + " > 1e309";
    return out;
}

std::string extractMetricNameFromExpression(const std::string& expr) {
    if (expr.empty()) return std::string();
    static const std::vector<std::string> ops = {">=","<=","==","!=",">","<"};
    size_t pos = std::string::npos;
    for (const auto& o : ops) { auto p = expr.find(o); if (p != std::string::npos) { pos = p; break; } }
    std::string lhs = pos == std::string::npos ? expr : expr.substr(0, pos);
    lhs = trimStr(lhs);
    std::vector<std::string> tokens; tokens.reserve(3);
    {
        std::string buf;
        for (char ch : lhs) {
            if (ch == '.') { if (!buf.empty()) { tokens.push_back(buf); buf.clear(); } }
            else buf.push_back(ch);
        }
        if (!buf.empty()) tokens.push_back(buf);
    }
    if (tokens.empty()) return std::string();
    if (tokens.size() >= 2) return tokens[1];
    return tokens.back();
}

web::UserAlertRule toUserAlertRule(const alert::Rule& r) {
    web::UserAlertRule ur;
    ur.alert_name = r.id;
    ur.alert_type = r.tag;
    ur.created_at = r.created_at;
    ur.description = r.description;
    ur.enabled = r.enabled;
    ur.expression = ::yw::web::UserRuleExpression{};
    ur.expression = parseExpressionObject(r.expression).get<::yw::web::UserRuleExpression>();
    if (!r.selector.empty()) {
        ur.expression.tags.clear();
        ur.expression.tags.reserve(1);
        alert::LabelSet t; t.reserve(r.selector.size());
        for (const auto& kv : r.selector) t[kv.first] = kv.second;
        ur.expression.tags.push_back(std::move(t));
    }
    long long every = parseDurationSeconds(r.eval_every);
    long long total = (every <= 0 ? 0 : every * (r.for_times <= 0 ? 1 : r.for_times));
    ur.for_duration = formatSecondsCompact(total);
    ur.id = r.id;
    try { ur.severity = nlohmann::json(r.severity).get<std::string>(); }
    catch (...) { ur.severity = ""; }
    ur.summary = r.name;
    ur.updated_at = r.updated_at;
    return ur;
}

alert::Rule fromUserAlertRule(const web::UserAlertRule& ur) {
    alert::Rule r;
    r.id = ur.id.empty() ? ur.alert_name : ur.id;
    r.name = ur.summary.empty() ? r.id : ur.summary;
    r.description = ur.description;
    r.enabled = ur.enabled;
    {
        nlohmann::json exprObj = nlohmann::json{
            {"stable", ur.expression.stable},
            {"metric", ur.expression.metric},
            {"conditions", nlohmann::json::array()}
        };
        for (const auto& c : ur.expression.conditions) {
            exprObj["conditions"].push_back({ {"operator", c.op}, {"threshold", c.threshold} });
        }
        r.expression = buildExpressionString(exprObj);
    }
    r.eval_every = "1s";
    r.selector.clear();
    if (!ur.expression.tags.empty()) {
        for (const auto& obj : ur.expression.tags) {
            for (const auto& kv : obj) r.selector[kv.first] = kv.second;
        }
    }
    r.tag = ur.alert_type;
    if (ur.severity == "提示") r.severity = alert::Severity::Info;
    else if (ur.severity == "严重") r.severity = alert::Severity::Critical;
    else r.severity = alert::Severity::Warn;
    r.window = "5m";
    r.for_times = 1;
    return r;
}

web::UserAlertEventView toUserAlertEventView(const alert::AlertEvent& e) {
    web::UserAlertEventView v;
    v.annotations.description = e.description;
    v.annotations.summary = e.title;
    v.created_at = formatTimestampMs(e.timestamp_ms);
    v.ends_at = formatTimestampMs(e.resolved_timestamp_ms);
    v.fingerprint = e.fingerprint;
    v.id = e.fingerprint;
    v.labels.clear();
    if (e.context.contains("tag") && e.context["tag"].is_string()) v.labels["alert_type"] = e.context["tag"].get<std::string>();
    else v.labels["alert_type"] = "metric";
    if (e.context.contains("rule_id") && e.context["rule_id"].is_string()) v.labels["alertname"] = e.context["rule_id"].get<std::string>();
    else v.labels["alertname"] = e.rule_id;
    auto it = e.labels.find("host_ip");
    if (it != e.labels.end()) v.labels["host_ip"] = it->second;
    else if (e.context.contains("host_ip") && e.context["host_ip"].is_string()) v.labels["host_ip"] = e.context["host_ip"].get<std::string>();
    {
        auto hip = v.labels.find("host_ip");
        if (hip != v.labels.end()) {
            std::string& s = hip->second;
            if (s.size() >= 3 && s.compare(s.size()-3, 3, "/32") == 0) {
                s.erase(s.size()-3);
            }
        }
    }
    {
        std::string metric_name = extractMetricNameFromExpression(e.context.value("metric", std::string()));
        if (!metric_name.empty()) v.labels["metrics"] = metric_name;
    }
    std::string sev = "一般";
    switch (e.severity) {
        case alert::Severity::Info: sev = "提示"; break;
        case alert::Severity::Critical: sev = "严重"; break;
        case alert::Severity::Warn: default: sev = "一般"; break;
    }
    v.labels["severity"] = sev;
    {
        std::ostringstream vss; vss << std::fixed << std::setprecision(6) << e.value;
        v.labels["value"] = vss.str();
    }
    v.starts_at = v.created_at;
    switch (e.status) {
        case alert::AlertStatus::Pending: v.status = "pending"; break;
        case alert::AlertStatus::Firing: v.status = "firing"; break;
        case alert::AlertStatus::Resolved: v.status = "resolved"; break;
        case alert::AlertStatus::Inactive: default: v.status = "inactive"; break;
    }

    v.updated_at = v.ends_at.empty() ? v.created_at : v.ends_at;

    {
        alert::LabelSet placeholders = v.labels;
        for (const auto& kv : e.labels) {
            if (placeholders.find(kv.first) == placeholders.end()) placeholders[kv.first] = kv.second;
        }
        if (e.context.is_object()) {
            for (auto itc = e.context.begin(); itc != e.context.end(); ++itc) {
                if (itc.value().is_string()) placeholders[itc.key()] = itc.value().get<std::string>();
                else if (itc.value().is_number()) placeholders[itc.key()] = itc.value().dump();
            }
        }
        v.annotations.description = renderDescription(e.description, placeholders);
    }

    return v;
}

std::vector<web::UserAlertEventView> toUserAlertEventViews(const std::vector<alert::AlertEvent>& es) {
    std::vector<web::UserAlertEventView> out;
    out.reserve(es.size());
    for (const auto& e : es) out.push_back(toUserAlertEventView(e));
    return out;
}

} // namespace mapper
} // namespace web
} // namespace yw




