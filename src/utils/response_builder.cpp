#include "utils/response_builder.h"

namespace yw {
namespace utils {

void ResponseBuilder::sendSuccess(const HttpContextPtr& ctx, const json& data) {
    json resp = {
        {"api_version", ApiVersion::V2},
        {"status", "success"},
        {"data", data}
    };
    sendJson(ctx, resp);
}

void ResponseBuilder::sendError(const HttpContextPtr& ctx, 
                                const std::string& message,
                                http_status status_code,
                                const json& data) {
    json resp = {
        {"api_version", ApiVersion::V2},
        {"status", "error"},
        {"message", message},
        {"data", data}
    };
    ctx->setStatus(status_code);
    sendJson(ctx, resp);
}

void ResponseBuilder::sendErrorLegacy(const HttpContextPtr& ctx,
                                      http_status code,
                                      const std::string& message,
                                      const json& data) {
    json resp = {
        {"code", code},
        {"message", message},
        {"data", data}
    };
    ctx->setStatus(code);
    sendJson(ctx, resp);
}

int ResponseBuilder::sendErrorLegacyWithReturn(const HttpContextPtr& ctx,
                                               http_status code,
                                               const std::string& message,
                                               const json& data) {
    json resp = {
        {"code", code},
        {"message", message},
        {"data", data}
    };
    ctx->setStatus(code);
    return sendJson(ctx, resp);
}

int ResponseBuilder::sendJson(const HttpContextPtr& ctx, const json& json_obj, int indent) {
    ctx->setContentType("application/json");
    return ctx->send(json_obj.dump(indent));
}

int ResponseBuilder::sendSuccessWithReturn(const HttpContextPtr& ctx, const json& data) {
    json resp = {
        {"api_version", ApiVersion::V2},
        {"status", "success"},
        {"data", data}
    };
    return sendJson(ctx, resp);
}

int ResponseBuilder::sendErrorWithReturn(const HttpContextPtr& ctx, 
                                        const std::string& message,
                                        http_status status_code,
                                        const json& data) {
    json resp = {
        {"api_version", ApiVersion::V2},
        {"status", "error"},
        {"message", message},
        {"data", data}
    };
    ctx->setStatus(status_code);
    return sendJson(ctx, resp);
}

std::vector<std::string> ResponseBuilder::parseCommaSeparated(const std::string& param_value) {
    std::vector<std::string> result;
    if (param_value.empty()) {
        return result;
    }

    std::string current_item;
    for (size_t i = 0; i <= param_value.size(); ++i) {
        if (i == param_value.size() || param_value[i] == ',') {
            std::string trimmed = trim(current_item);
            if (!trimmed.empty()) {
                result.push_back(std::move(trimmed));
            }
            current_item.clear();
        } else {
            current_item.push_back(param_value[i]);
        }
    }
    return result;
}

std::string ResponseBuilder::trim(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace utils
} // namespace yw

