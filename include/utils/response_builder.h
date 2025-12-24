#pragma once

#include <nlohmann/json.hpp>
#include <hv/HttpContext.h>
#include <hv/httpdef.h>
#include <string>
#include <cstdint>
#include <vector>

namespace yw {
namespace utils {

using json = nlohmann::json;
using HttpContextPtr = std::shared_ptr<hv::HttpContext>;

/**
 * @brief API 版本常量
 */
namespace ApiVersion {
    constexpr std::int32_t V1 = 1;  // API 版本 1
    constexpr std::int32_t V2 = 2;  // API 版本 2
}

/**
 * @brief HTTP 响应构建器
 * 
 * 提供统一的 JSON 响应构建和发送功能，默认使用 API 版本 2。
 */
class ResponseBuilder {
public:
    /**
     * @brief 构建并发送成功响应（默认 API 版本 2）
     * 
     * @param ctx HTTP 上下文
     * @param data 响应数据（JSON 对象）
     */
    static void sendSuccess(const HttpContextPtr& ctx, const json& data);

    /**
     * @brief 构建并发送错误响应（默认 API 版本 2）
     * 
     * @param ctx HTTP 上下文
     * @param message 错误消息
     * @param status_code HTTP 状态码
     * @param data 可选的错误数据（默认为空对象）
     */
    static void sendError(const HttpContextPtr& ctx, 
                         const std::string& message,
                         http_status status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR,
                         const json& data = json::object());

    /**
     * @brief 构建并发送错误响应（使用旧版格式，code 字段）
     * 
     * @param ctx HTTP 上下文
     * @param code HTTP 状态码
     * @param message 错误消息
     * @param data 响应数据（默认为空数组）
     */
    static void sendErrorLegacy(const HttpContextPtr& ctx,
                               http_status code,
                               const std::string& message,
                               const json& data = json::array());

    /**
     * @brief 构建并发送错误响应（使用旧版格式，code 字段）
     * 
     * @param ctx HTTP 上下文
     * @param code HTTP 状态码
     * @param message 错误消息
     * @param data 响应数据（默认为空数组）
     * @return HTTP 响应状态码
     */
    static int sendErrorLegacyWithReturn(const HttpContextPtr& ctx,
                                         http_status code,
                                         const std::string& message,
                                         const json& data = json::array());

    /**
     * @brief 发送 JSON 响应
     * 
     * @param ctx HTTP 上下文
     * @param json_obj JSON 对象
     * @param indent 缩进空格数（默认 2）
     * @return HTTP 响应状态码
     */
    static int sendJson(const HttpContextPtr& ctx, const json& json_obj, int indent = 2);

    /**
     * @brief 构建并发送成功响应（默认 API 版本 2）
     * 
     * @param ctx HTTP 上下文
     * @param data 响应数据（JSON 对象）
     * @return HTTP 响应状态码
     */
    static int sendSuccessWithReturn(const HttpContextPtr& ctx, const json& data);

    /**
     * @brief 构建并发送错误响应（默认 API 版本 2）
     * 
     * @param ctx HTTP 上下文
     * @param message 错误消息
     * @param status_code HTTP 状态码
     * @param data 可选的错误数据（默认为空对象）
     * @return HTTP 响应状态码
     */
    static int sendErrorWithReturn(const HttpContextPtr& ctx, 
                                   const std::string& message,
                                   http_status status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR,
                                   const json& data = json::object());

    /**
     * @brief 验证必需参数是否存在
     * 
     * @param params 参数映射（支持 std::map 和 std::unordered_map）
     * @param param_name 参数名
     * @return true 如果参数存在且非空
     */
    template<typename MapType>
    static bool hasParam(const MapType& params, const std::string& param_name) {
        auto it = params.find(param_name);
        return it != params.end() && !it->second.empty();
    }

    /**
     * @brief 获取参数值，如果不存在返回默认值
     * 
     * @param params 参数映射（支持 std::map 和 std::unordered_map）
     * @param param_name 参数名
     * @param default_value 默认值
     * @return 参数值或默认值
     */
    template<typename MapType>
    static std::string getParam(const MapType& params,
                                const std::string& param_name,
                                const std::string& default_value = "") {
        auto it = params.find(param_name);
        if (it != params.end() && !it->second.empty()) {
            return trim(it->second);
        }
        return default_value;
    }

    /**
     * @brief 解析整数参数
     * 
     * @param params 参数映射（支持 std::map 和 std::unordered_map）
     * @param param_name 参数名
     * @param default_value 默认值
     * @return 解析后的整数值，如果解析失败返回默认值
     */
    template<typename MapType>
    static int getIntParam(const MapType& params,
                          const std::string& param_name,
                          int default_value = 0) {
        auto it = params.find(param_name);
        if (it != params.end() && !it->second.empty()) {
            try {
                return std::stoi(it->second);
            } catch (...) {
                return default_value;
            }
        }
        return default_value;
    }

    /**
     * @brief 解析逗号分隔的字符串列表
     * 
     * @param param_value 参数字符串
     * @return 字符串列表
     */
    static std::vector<std::string> parseCommaSeparated(const std::string& param_value);

private:
    /**
     * @brief 去除字符串首尾空白字符
     * 
     * @param str 输入字符串
     * @return 去除空白后的字符串
     */
    static std::string trim(const std::string& str);
};

} // namespace utils
} // namespace yw
