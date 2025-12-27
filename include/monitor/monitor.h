// ============================================================================
// 文件功能描述：
// 监控模块（IMonitorModule）的头文件，定义资源监控模块的公共接口。
// 主要功能包括：
// 1. 资源查询接口：提供查询节点最新资源快照的接口
// 2. 指标序列查询：提供查询节点历史监控指标时序数据的接口
// 3. 数据导出：提供导出节点历史数据的功能接口
// 4. 工厂模式：提供MonitorFactory工厂类，用于创建监控模块实例
// 5. 接口抽象：定义IMonitorModule接口，隐藏具体实现细节，实现模块间的松耦合
// ============================================================================

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "monitor/monitor_model.h"

// 前向声明
namespace hv { struct HttpService; }

namespace yw {
namespace monitor {

struct Resource; // 前向声明，避免在公共头包含私有实现头

/**
 * @brief 监控模块接口
 * 
 * 提供节点资源监控和历史数据查询功能，包括资源快照、指标序列查询和数据导出等。
 */
class IMonitorModule {
public:
    virtual ~IMonitorModule() = default;

    // ========== 资源查询 ==========

    /**
     * @brief 获取指定节点的最近一次资源快照
     * 
     * @param host_ip 节点IP地址
     * @return 节点的资源快照，包含CPU、内存、网络、磁盘、GPU等资源信息；如果节点不存在或未上报数据返回 nullptr
     */
    virtual std::shared_ptr<Resource> getNodeResource(const std::string& host_ip) const = 0;

    // ========== 指标序列查询 ==========

    /**
     * @brief 查询指定节点在指定时间范围内的指标序列数据
     * 
     * @param host_ip 节点IP地址
     * @param duration 时间范围，支持格式：
     *   - 简写格式：数字+单位（如 "1h", "5m", "30s"）
     *   - PostgreSQL interval 格式（如 "1 hour", "5 minutes"）
     * @param kinds 指标类型列表，可选值：cpu、memory、network、disk、gpu；空向量表示查询全部类型
     * @return 指标序列数据，包含CPU、内存、网络、磁盘、GPU等各类指标的时序数据
     */
    virtual MetricsSeries queryMetricsSeries(const std::string& host_ip,
                                             const std::string& duration,
                                             const std::vector<std::string>& kinds) const = 0;

    // ========== 数据导出 ==========

    /**
     * @brief 导出节点历史数据（按 export.md 格式）
     * 
     * 将指定时间范围内的节点监控数据导出为统一格式，用于数据分析和报表生成。
     * 
     * @param host_ip 节点IP地址
     * @param start_time 开始时间（秒级时间戳）
     * @param end_time 结束时间（秒级时间戳）
     * @param types 要导出的指标类型列表，可选值：cpu、memory、network、disk、gpu；空向量表示导出全部类型
     * @return 导出数据，包含时间范围、节点IP、指标类型和数据点列表
     */
    virtual ExportData exportNodeData(const std::string& host_ip,
                                      std::int64_t start_time,
                                      std::int64_t end_time,
                                      const std::vector<std::string>& types) const = 0;
};

/**
 * @brief 监控模块工厂类
 * 
 * 负责创建和获取监控模块实例。
 */
class MonitorFactory {
public:
    /**
     * @brief 创建监控模块实例
     * 
     * @param service HTTP服务实例，用于接收节点上报的监控数据
     * @return 监控模块的共享指针，如果创建失败可能返回 nullptr
     */
    static std::shared_ptr<IMonitorModule> getMonitorModule(
        std::shared_ptr<hv::HttpService> service);
};

} // namespace monitor
} // namespace yw


