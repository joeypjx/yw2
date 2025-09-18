#include "AlertRoutes.h"

#include <nlohmann/json.hpp>
#include "mapper/AlertMapper.h"

namespace yw {
namespace web {
namespace routes {

using json = nlohmann::json;

void registerAlertRoutes(hv::HttpService* service,
                         alert::IAlertModule* alert_module,
                         AlertPusher* /*pusher*/) {
    if (!service) return;

    // POST /alarm/rules
    service->POST("/alarm/rules", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            nlohmann::json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        try {
            auto body = ctx->body();
            if (body.empty()) {
                json resp = {{"api_version",1},{"status","error"},{"message","empty request body"},{"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
            auto rule_json = json::parse(body);
            web::UserAlertRule ur = rule_json;
            if (ur.alert_name.empty() && ur.id.empty()) {
                json resp = {{"api_version",1},{"status","error"},{"message","missing alert_name/id"},{"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
            alert::Rule rule = yw::web::mapper::fromUserAlertRule(ur);
            rule.window = rule_json.value("window", rule.window);
            rule.eval_every = rule_json.value("eval_every", rule.eval_every);
            int ft = rule_json.value("for_times", 0);
            if (rule_json.contains("for") && rule_json["for"].is_string()) {
                long long for_seconds = yw::web::mapper::parseDurationSeconds(rule_json["for"].get<std::string>());
                long long every_seconds = yw::web::mapper::parseDurationSeconds(rule.eval_every);
                if (every_seconds <= 0) every_seconds = 1;
                long long quotient = (for_seconds + every_seconds - 1) / every_seconds;
                ft = static_cast<int>(quotient);
            }
            if (ft <= 0) ft = 1; rule.for_times = ft;
            rule.enabled = ur.enabled;
            if (alert_module->upsertRule(rule)) {
                json resp = {{"api_version",1},{"data",{{"id",rule.id},{"message","Rule created/updated successfully"}}},{"status","success"}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            }
            json resp = {{"api_version",1},{"status","error"},{"message","failed to create/update rule"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        } catch (const json::exception& e) {
            json resp = {{"api_version",1},{"status","error"},{"message","invalid JSON format: " + std::string(e.what())},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            json resp = {{"api_version",1},{"status","error"},{"message","internal error: " + std::string(e.what())},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // GET /alarm/rules
    service->GET("/alarm/rules", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        auto rules = alert_module->listRules();
        nlohmann::json arr = nlohmann::json::array();
        arr.get_ref<nlohmann::json::array_t&>().reserve(rules.size());
        for (const auto& r : rules) arr.push_back(yw::web::mapper::toUserAlertRule(r));
        nlohmann::json resp = {{"api_version",1},{"data",arr},{"status","success"}};
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // GET /alarm/rules/{id}
    service->GET("/alarm/rules/{id}", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        const std::string rid = ctx->param("id");
        auto ruleOpt = alert_module->getRule(rid);
        if (!ruleOpt) {
            json resp = {{"api_version",1},{"status","error"},{"message","rule not found"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_NOT_FOUND);
            return ctx->send(resp.dump(2));
        }
        const auto& r = *ruleOpt;
        nlohmann::json item = yw::web::mapper::toUserAlertRule(r);
        nlohmann::json resp = {{"api_version",1},{"data",item},{"status","success"}};
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // POST /alarm/rules/{id}/update
    service->POST("/alarm/rules/{id}/update", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        const std::string rid = ctx->param("id");
        try {
            auto body = ctx->body();
            if (body.empty()) {
                json resp = {{"api_version",1},{"status","error"},{"message","empty request body"},{"data", json::object()}};
                ctx->setContentType("application/json");
                ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
                return ctx->send(resp.dump(2));
            }
            auto rule_json = nlohmann::json::parse(body);
            web::UserAlertRule ur = rule_json;
            if (ur.alert_name.empty() && ur.id.empty()) { ur.id = rid; ur.alert_name = rid; }
            alert::Rule rule = yw::web::mapper::fromUserAlertRule(ur);
            rule.id = rid;
            rule.window = rule_json.value("window", rule.window);
            rule.eval_every = rule_json.value("eval_every", rule.eval_every);
            int ft = rule_json.value("for_times", 0);
            if (rule_json.contains("for") && rule_json["for"].is_string()) {
                long long for_seconds = yw::web::mapper::parseDurationSeconds(rule_json["for"].get<std::string>());
                long long every_seconds = yw::web::mapper::parseDurationSeconds(rule.eval_every);
                if (every_seconds <= 0) every_seconds = 1;
                long long quotient = (for_seconds + every_seconds - 1) / every_seconds;
                ft = static_cast<int>(quotient);
            }
            if (ft <= 0) ft = 1; rule.for_times = ft;
            rule.enabled = ur.enabled;
            if (alert_module->upsertRule(rule)) {
                nlohmann::json resp = {{"api_version",1},{"data",{{"id",rule.id},{"message","Rule updated successfully"}}},{"status","success"}};
                ctx->setContentType("application/json");
                return ctx->send(resp.dump(2));
            }
            json resp = {{"api_version",1},{"status","error"},{"message","failed to update rule"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        } catch (const nlohmann::json::exception& e) {
            json resp = {{"api_version",1},{"status","error"},{"message","invalid JSON format: " + std::string(e.what())},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            json resp = {{"api_version",1},{"status","error"},{"message","internal error: " + std::string(e.what())},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
    });

    // POST /alarm/rules/{id}/delete
    service->POST("/alarm/rules/{id}/delete", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        const std::string id = ctx->param("id");
        if (alert_module->deleteRule(id)) {
            json resp = {{"api_version",1},{"status","success"},{"data", {{"id", id}}}};
            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        }
        json resp = {{"api_version",1},{"status","error"},{"message","failed to delete rule"},{"data", json::object()}};
        ctx->setContentType("application/json");
        ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return ctx->send(resp.dump(2));
    });

    // GET /alarm/events
    service->GET("/alarm/events", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        std::string duration = "24h";
        auto params = ctx->params();
        if (params.find("duration") != params.end()) duration = params["duration"];
        auto events = alert_module->queryEvents(duration);
        auto views = yw::web::mapper::toUserAlertEventViews(events);
        nlohmann::json arr = nlohmann::json::array();
        arr.get_ref<nlohmann::json::array_t&>().reserve(views.size());
        for (const auto& v : views) arr.push_back(v);
        json resp = {{"api_version",1},{"data",arr},{"status","success"}};
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // GET /alarm/count
    service->GET("/alarm/count", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        std::string status_str = ctx->param("status");
        if (status_str.empty()) {
            json resp = {{"api_version",1},{"status","error"},{"message","missing status"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        for (auto& ch : status_str) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        alert::AlertStatus st;
        if (status_str == "pending") st = alert::AlertStatus::Pending;
        else if (status_str == "firing") st = alert::AlertStatus::Firing;
        else if (status_str == "resolved") st = alert::AlertStatus::Resolved;
        else if (status_str == "inactive") st = alert::AlertStatus::Inactive;
        else {
            json resp = {{"api_version",1},{"status","error"},{"message","invalid status (pending|firing|resolved|inactive)"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        std::size_t count = 0; try { count = alert_module->countEventsByStatus(st); } catch(...) {
            json resp = {{"api_version",1},{"status","error"},{"message","internal error"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        nlohmann::json resp = {{"api_version",1},{"data",{{"status",status_str},{"count",count}}},{"status","success"}};
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // POST /alarm/component
    service->POST("/alarm/component", [alert_module](const HttpContextPtr& ctx) {
        if (!alert_module) {
            json resp = {{"api_version",1},{"status","error"},{"message","alert module unavailable"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return ctx->send(resp.dump(2));
        }
        auto body = ctx->body();
        if (body.empty()) {
            json resp = {{"api_version",1},{"status","error"},{"message","empty request body"},{"data", json::object()}};
            ctx->setContentType("application/json");
            ctx->setStatus(HTTP_STATUS_BAD_REQUEST);
            return ctx->send(resp.dump(2));
        }
        auto j = nlohmann::json::parse(body);
        alert::AlertEvent event;
        event.fingerprint = j.value("host_ip", "") + "_" + j.value("instance_id", "") + "_" + j.value("uuid", "") + "_" + j.value("index", "");
        event.rule_id = j.value("rule_id", "component");
        event.action = "firing";
        event.status = alert::AlertStatus::Firing;
        event.severity = alert::Severity::Warn;
        event.context = j;
        event.title = "业务组件状态异常";
        event.description = j.value("host_ip", "") + " 节点上 " + j.value("instance_id", "") + " 组件状态为 " + j.value("status", "unknown");
        event.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count();
        alert_module->appendAlertEvent(event);
        json resp2 = {{"api_version",1},{"status","success"},{"data", {{"fingerprint", event.fingerprint}}}};
        ctx->setContentType("application/json");
        return ctx->send(resp2.dump(2));
    });
}

} // namespace routes
} // namespace web
} // namespace yw


