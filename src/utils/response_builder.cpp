// ============================================================================
// 文件功能描述：
// HTTP响应构建器（ResponseBuilder）的实现文件，提供统一的HTTP响应格式化和发送功能。
// 主要功能包括：
// 1. 成功响应构建：构建标准格式的成功响应，包含api_version、status和data字段
// 2. 错误响应构建：构建标准格式的错误响应，包含api_version、status、message和data字段
// 3. 旧版格式兼容：提供旧版API格式的响应构建方法，保持向后兼容
// 4. JSON格式化：支持紧凑格式和格式化输出（可配置缩进空格数）
// 5. 参数解析：提供解析逗号分隔参数字符串的功能（如"cpu,memory,disk"）
// 6. 字符串工具：提供去除首尾空白字符的功能
// 7. 返回值支持：提供void和int返回类型的响应方法，适配不同的路由处理函数签名
// ============================================================================

#include "utils/response_builder.h"

namespace yw {
namespace utils {

// 发送成功响应（不返回值，用于void返回类型的路由处理函数）
// ctx: HTTP上下文
// data: 响应数据（JSON对象）
void ResponseBuilder::sendSuccess(const HttpContextPtr& ctx, const json& data) {
    json resp = {
        {"api_version", ApiVersion::V2},
        {"status", "success"},
        {"data", data}
    };
    sendJson(ctx, resp);
}

// 发送错误响应（不返回值，用于void返回类型的路由处理函数）
// ctx: HTTP上下文
// message: 错误消息
// status_code: HTTP状态码（如400、500等）
// data: 可选的错误数据（JSON对象）
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

// 发送旧版格式的错误响应（兼容旧API）
// ctx: HTTP上下文
// code: HTTP状态码
// message: 错误消息
// data: 可选的错误数据
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

// 发送旧版格式的错误响应并返回状态码（用于需要返回值的路由处理函数）
// ctx: HTTP上下文
// code: HTTP状态码
// message: 错误消息
// data: 可选的错误数据
// 返回: HTTP响应状态码
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

// 发送JSON响应
// ctx: HTTP上下文
// json_obj: JSON对象
// indent: JSON格式化缩进空格数（默认-1表示紧凑格式）
// 返回: HTTP响应状态码
int ResponseBuilder::sendJson(const HttpContextPtr& ctx, const json& json_obj, int indent) {
    ctx->setContentType("application/json");
    return ctx->send(json_obj.dump(indent));
}

// 发送成功响应并返回状态码（用于需要返回值的路由处理函数）
// ctx: HTTP上下文
// data: 响应数据（JSON对象）
// 返回: HTTP响应状态码
int ResponseBuilder::sendSuccessWithReturn(const HttpContextPtr& ctx, const json& data) {
    json resp = {
        {"api_version", ApiVersion::V2},
        {"status", "success"},
        {"data", data}
    };
    return sendJson(ctx, resp);
}

// 发送错误响应并返回状态码（用于需要返回值的路由处理函数）
// ctx: HTTP上下文
// message: 错误消息
// status_code: HTTP状态码
// data: 可选的错误数据
// 返回: HTTP响应状态码
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

// 解析逗号分隔的参数字符串
// param_value: 逗号分隔的字符串（如"cpu,memory,disk"）
// 返回: 解析后的字符串数组，自动去除空白字符
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

// 去除字符串首尾的空白字符（空格、制表符、换行符等）
// str: 待处理的字符串
// 返回: 去除空白后的字符串，如果全为空白则返回空字符串
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

