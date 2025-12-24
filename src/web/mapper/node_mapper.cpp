#include "node_mapper.h"

namespace yw {
namespace web {
namespace mapper {

namespace {
// 常量定义
constexpr const char* CONTAINER_STATE_RUNNING = "RUNNING";
constexpr const char* CONTAINER_STATE_PAUSED = "PAUSED";
constexpr const char* CONTAINER_STATE_STOPPED = "STOPPED";
constexpr const char* NODE_STATUS_OFFLINE = "offline";
constexpr const char* NODE_STATUS_OFFLINE_IN_POSITION = "offline_in_position";

// 辅助函数：应用节点状态特殊逻辑
inline void applyNodeStatusLogic(std::string& status, const std::optional<std::uint8_t>& prst) {
  if (status == NODE_STATUS_OFFLINE && prst.has_value() && prst.value() != 0) {
    status = NODE_STATUS_OFFLINE_IN_POSITION;
  }
}

// 辅助函数：复制 NodeExt 基础字段到 DTO
template<typename DTO>
void copyNodeExtBaseFields(DTO& dto, const node::NodeExt& ext) {
  dto.box_id = ext.box_id;
  dto.cpu_id = ext.cpu_id;
  dto.slot_id = ext.slot_id;
  dto.host_ip = ext.host_ip;
  dto.status = ext.status;
  dto.updated_at = ext.updated_at;
}

// 辅助函数：转换容器统计
LatestContainerMetrics convertContainerMetrics(const std::vector<monitor::ComponentEntry>& components, std::int64_t timestamp) {
  LatestContainerMetrics metrics;
  metrics.container_count = static_cast<int>(components.size());
  
  int running = 0, paused = 0, stopped = 0;
  for (const auto& c : components) {
    if (c.state == CONTAINER_STATE_RUNNING) {
      running++;
    } else if (c.state == CONTAINER_STATE_PAUSED) {
      paused++;
    } else if (c.state == CONTAINER_STATE_STOPPED) {
      stopped++;
    }
  }
  
  metrics.running_count = running;
  metrics.paused_count = paused;
  metrics.stopped_count = stopped;
  metrics.timestamp = timestamp;
  return metrics;
}

// 辅助函数：转换 CPU 指标
LatestCpuMetrics convertCpuMetrics(const monitor::CpuResource& cpu, std::int64_t timestamp) {
  LatestCpuMetrics metrics;
  metrics.core_allocated = cpu.core_allocated;
  metrics.core_count = cpu.core_count;
  metrics.current = cpu.current;
  metrics.load_avg_1m = cpu.load_avg_1m;
  metrics.load_avg_5m = cpu.load_avg_5m;
  metrics.load_avg_15m = cpu.load_avg_15m;
  metrics.power = cpu.power;
  metrics.temperature = cpu.temperature;
  metrics.usage_percent = cpu.usage_percent;
  metrics.voltage = cpu.voltage;
  metrics.timestamp = timestamp;
  return metrics;
}

// 辅助函数：转换内存指标
LatestMemoryMetrics convertMemoryMetrics(const monitor::MemoryResource& mem, std::int64_t timestamp) {
  LatestMemoryMetrics metrics;
  metrics.total = mem.total;
  metrics.used = mem.used;
  metrics.free = mem.free;
  metrics.usage_percent = mem.usage_percent;
  metrics.timestamp = timestamp;
  return metrics;
}

// 辅助函数：转换网络指标
LatestNetworkMetrics convertNetworkMetrics(const std::vector<monitor::NetworkInterface>& networks, std::int64_t timestamp) {
  LatestNetworkMetrics metrics;
  metrics.network_count = static_cast<int>(networks.size());
  metrics.networks.clear();
  metrics.networks.reserve(networks.size());
  
  for (const auto& ni : networks) {
    NetworkSample s;
    s.interface = ni.interface;
    s.rx_bytes = ni.rx_bytes;
    s.tx_bytes = ni.tx_bytes;
    s.rx_packets = ni.rx_packets;
    s.tx_packets = ni.tx_packets;
    s.rx_errors = ni.rx_errors;
    s.tx_errors = ni.tx_errors;
    s.rx_rate = ni.rx_rate;
    s.tx_rate = ni.tx_rate;
    s.rx_drop_rate = ni.rx_drop_rate;
    s.tx_drop_rate = ni.tx_drop_rate;
    s.state = ni.state;
    s.timestamp = timestamp;
    metrics.networks.push_back(std::move(s));
  }
  
  metrics.timestamp = timestamp;
  return metrics;
}

// 辅助函数：转换磁盘指标
LatestDiskMetrics convertDiskMetrics(const std::vector<monitor::DiskPartition>& disks, std::int64_t timestamp) {
  LatestDiskMetrics metrics;
  metrics.disk_count = static_cast<int>(disks.size());
  metrics.disks.clear();
  metrics.disks.reserve(disks.size());
  
  for (const auto& dk : disks) {
    DiskSample d;
    d.device = dk.device;
    d.mount_point = dk.mount_point;
    d.total = dk.total;
    d.used = dk.used;
    d.free = dk.free;
    d.usage_percent = dk.usage_percent;
    d.timestamp = timestamp;
    metrics.disks.push_back(std::move(d));
  }
  
  metrics.timestamp = timestamp;
  return metrics;
}

// 辅助函数：转换 GPU 指标
LatestGpuMetrics convertGpuMetrics(const std::vector<monitor::GpuResource>& gpus, std::int64_t timestamp) {
  LatestGpuMetrics metrics;
  metrics.gpu_count = static_cast<int>(gpus.size());
  metrics.gpus.clear();
  metrics.gpus.reserve(gpus.size());
  
  for (const auto& g : gpus) {
    GpuSample gs;
    gs.index = g.index;
    gs.name = g.name;
    gs.compute_usage = g.compute_usage;
    gs.mem_usage = g.mem_usage;
    gs.mem_used = g.mem_used;
    gs.mem_total = g.mem_total;
    gs.temperature = g.temperature;
    gs.power = g.power;
    gs.current = 0.0; // 未上报，默认 0
    gs.voltage = 0.0; // 未上报，默认 0
    gs.timestamp = timestamp;
    metrics.gpus.push_back(std::move(gs));
  }
  
  metrics.timestamp = timestamp;
  return metrics;
}

// 辅助函数：转换 BMC 传感器数据
LatestSensorMetrics convertBmcSensors(const std::unordered_map<std::string, bmc::BMCSensorRow>& sensors, std::int64_t timestamp) {
  LatestSensorMetrics metrics;
  metrics.sensor_count = static_cast<int>(sensors.size());
  metrics.sensors.clear();
  metrics.sensors.reserve(sensors.size());
  
  for (const auto& [sensorname, sensor_row] : sensors) {
    nlohmann::json sensor_json;
    sensor_json["timestamp"] = sensor_row.timestamp;
    sensor_json["host_ip"] = sensor_row.host_ip;
    sensor_json["sensorseq"] = sensor_row.sensorseq;
    sensor_json["sensortype"] = sensor_row.sensortype;
    sensor_json["sensorname"] = sensor_row.sensorname;
    sensor_json["sensorvalue_L"] = sensor_row.sensorvalue_L;
    sensor_json["sensorvalue_H"] = sensor_row.sensorvalue_H;
    sensor_json["sensor_value"] = sensor_row.sensor_value;
    sensor_json["sensoralmtype"] = sensor_row.sensoralmtype;
    metrics.sensors.push_back(std::move(sensor_json));
  }
  
  metrics.timestamp = timestamp;
  return metrics;
}

} // anonymous namespace

NodeView toNodeView(const node::NodeExt &ext, const monitor::Resource *res,
                    const std::optional<std::uint8_t> prst) {
  NodeView v;
  v.box_id = ext.box_id;
  v.slot_id = ext.slot_id;
  v.cpu_id = ext.cpu_id;
  v.host_ip = ext.host_ip;
  v.hostname = ext.hostname;
  v.board_type = ext.board_type;
  v.box_type = ext.box_type;
  v.cpu_type = ext.cpu_type;
  v.os_type = ext.os_type;
  v.resource_type = ext.resource_type;
  v.cpu_arch = ext.cpu_arch;
  v.manufacturer = ext.manufacturer;
  v.serial_number = ext.serial_number;
  v.production_date = ext.production_date;

  // GPU 设备列表
  v.gpu.reserve(ext.gpu.size());
  for (const auto &g : ext.gpu) {
    GpuDevice gd;
    gd.index = g.index;
    gd.name = g.name;
    v.gpu.push_back(std::move(gd));
  }

  // 组件列表
  if (res) {
    v.component = res->component;
  } else {
    v.component.clear();
  }

  // 设置状态
  v.status = ext.status;

  // 应用状态特殊逻辑
  applyNodeStatusLogic(v.status, prst);

  return v;
}

NodeMetrics toNodeMetrics(
    const node::NodeExt &nx, const monitor::Resource *res,
    std::int64_t now_seconds,
    const std::unordered_map<std::string, bmc::BMCSensorRow> *bmc_sensors,
    const std::optional<std::uint8_t> prst) {
  NodeMetrics m;
  
  // 复制 NodeExt 基础字段
  copyNodeExtBaseFields(m, nx);
  
  // 应用状态特殊逻辑
  applyNodeStatusLogic(m.status, prst);

  if (res) {
    // 组件列表
    m.component = res->component;
    
    // 转换各项指标
    m.latest_container_metrics = convertContainerMetrics(res->component, now_seconds);
    m.latest_cpu_metrics = convertCpuMetrics(res->resource.cpu, now_seconds);
    m.latest_memory_metrics = convertMemoryMetrics(res->resource.memory, now_seconds);
    m.latest_network_metrics = convertNetworkMetrics(res->resource.network, now_seconds);
    m.latest_disk_metrics = convertDiskMetrics(res->resource.disk, now_seconds);
    m.latest_gpu_metrics = convertGpuMetrics(res->resource.gpu, now_seconds);
  }

  // BMC传感器数据（如果提供）
  if (bmc_sensors && !bmc_sensors->empty()) {
    m.latest_sensor_metrics = convertBmcSensors(*bmc_sensors, now_seconds);
  } else {
    // 传感器（暂无数据，保持空）
    m.latest_sensor_metrics.sensor_count = 0;
    m.latest_sensor_metrics.sensors.clear();
    m.latest_sensor_metrics.timestamp = now_seconds;
  }

  return m;
}

HistoricalMetricsView
toHistoricalMetricsView(const monitor::ResourceWindow &win,
                        const nlohmann::json *bmc_sensor) {
  HistoricalMetricsView v;
  
  // 节点标识信息
  v.box_id = win.box_id;
  v.cpu_id = win.cpu_id;
  v.host_ip = win.host_ip;
  v.slot_id = win.slot_id;
  v.time_range = win.time_range;
  
  // 历史指标数据
  v.metrics.cpu = win.metrics.cpu;
  v.metrics.disk = win.metrics.disk;
  v.metrics.gpu = win.metrics.gpu;
  v.metrics.network = win.metrics.network;
  v.metrics.memory = win.metrics.memory;
  
  // BMC传感器历史数据（可选）
  v.metrics.sensor = bmc_sensor ? *bmc_sensor : nlohmann::json::object();
  
  return v;
}

} // namespace mapper
} // namespace web
} // namespace yw
