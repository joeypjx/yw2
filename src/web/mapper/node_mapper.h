#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "node/node_model.h"
#include "monitor/monitor_model.h"
#include "bmc/bmc_model.h"
#include "dto/node_view_dto.h"
#include "dto/node_metrics_dto.h"
#include "dto/historical_metrics_dto.h"

namespace yw {
namespace web {
namespace mapper {

/**
 * @brief 将节点扩展信息转换为节点视图对象
 * 
 * 用于节点列表展示，包含节点基本信息和组件列表。
 * 如果节点状态为 "offline" 但存在在位信号（prst != 0），
 * 则状态会被转换为 "offline_in_position"。
 * 
 * @param ext 节点扩展信息（包含状态和时间戳）
 * @param res 资源信息（可为空，如果为空则组件列表为空）
 * @param prst 在位信号（可选，用于判断离线节点是否在位）
 * @return NodeView 节点视图对象
 */
NodeView toNodeView(const node::NodeExt& ext,
                    const monitor::Resource* res,
                    const std::optional<std::uint8_t> prst = std::nullopt);

/**
 * @brief 将节点信息转换为完整的监控指标视图
 * 
 * 用于节点详细指标展示（/node/metrics 接口），包含：
 * - 节点基础信息
 * - 容器统计（运行/暂停/停止数量）
 * - CPU、内存、网络、磁盘、GPU 实时指标
 * - BMC 传感器数据（可选）
 * 
 * @param ext 节点扩展信息
 * @param res 资源信息（包含各项监控指标）
 * @param now_seconds 当前时间戳（秒）
 * @param bmc_sensors BMC传感器数据映射（可选，键为传感器名称）
 * @param prst 在位信号（可选，用于判断离线节点是否在位）
 * @return NodeMetrics 节点指标视图对象
 */
NodeMetrics toNodeMetrics(const node::NodeExt& ext,
                          const monitor::Resource* res,
                          std::int64_t now_seconds,
                          const std::unordered_map<std::string, bmc::BMCSensorRow>* bmc_sensors = nullptr,
                          const std::optional<std::uint8_t> prst = std::nullopt);

/**
 * @brief 将历史监控窗口数据转换为历史指标视图
 * 
 * 用于历史指标查询（/node/historical-metrics 接口），包含：
 * - 节点标识信息
 * - 时间范围
 * - CPU、内存、网络、磁盘、GPU 历史指标序列
 * - BMC 传感器历史数据（可选）
 * 
 * @param win 资源窗口数据（包含时间范围和指标序列）
 * @param bmc_sensor BMC传感器历史数据（可选，JSON 格式）
 * @return HistoricalMetricsView 历史指标视图对象
 */
HistoricalMetricsView toHistoricalMetricsView(const monitor::ResourceWindow& win,
                                               const nlohmann::json* bmc_sensor = nullptr);

} // namespace mapper
} // namespace web
} // namespace yw