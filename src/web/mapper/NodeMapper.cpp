#include "NodeMapper.h"

namespace yw {
namespace web {
namespace mapper {

NodeView toNodeView(const node::NodeExt& ext, const monitor::Resource* res) {
    NodeView v;
    v.box_id = ext.box_id;
    v.slot_id = ext.slot_id;
    v.cpu_id = ext.cpu_id;
    v.host_ip = ext.host_ip;
    v.hostname = ext.hostname;
    v.status = ext.status;
    v.board_type = ext.board_type;
    v.box_type = ext.box_type;
    v.cpu_type = ext.cpu_type;
    v.os_type = ext.os_type;
    v.resource_type = ext.resource_type;
    v.cpu_arch = ext.cpu_arch;
    v.updated_at = ext.updated_at;
    v.manufacturer = ext.manufacturer;
    v.serial_number = ext.serial_number;
    v.production_date = ext.production_date;

    for (const auto& g : ext.gpu) {
        GpuDevice gd;
        gd.index = g.index;
        gd.name = g.name;
        v.gpu.push_back(std::move(gd));
    }
    
    if (res) {
        v.component = res->component;
    } else {
        v.component.clear();
    }
    return v;
}

NodeMetrics toNodeMetrics(const node::NodeExt& nx, const monitor::Resource* res, std::int64_t now_seconds,
                          const std::unordered_map<std::string, bmc::BMCSensorRow>* bmc_sensors) {
    NodeMetrics m;
    // NodeExt 基础
    m.box_id = nx.box_id;
    m.cpu_id = nx.cpu_id;
    m.slot_id = nx.slot_id;
    m.host_ip = nx.host_ip;
    m.status = nx.status;
    m.updated_at = nx.updated_at;

    if (res) {
        // 组件
        m.component = res->component;
        // 容器统计
        int running = 0, paused = 0, stopped = 0;
        for (const auto& c : res->component) {
            if (c.state == "RUNNING") running++;
            else if (c.state == "PAUSED") paused++;
            else if (c.state == "STOPPED") stopped++;
        }
        m.latest_container_metrics.container_count = (int)res->component.size();
        m.latest_container_metrics.running_count = running;
        m.latest_container_metrics.paused_count = paused;
        m.latest_container_metrics.stopped_count = stopped;
        m.latest_container_metrics.timestamp = now_seconds;

        // CPU
        const auto& cpu = res->resource.cpu;
        m.latest_cpu_metrics.core_allocated = cpu.core_allocated;
        m.latest_cpu_metrics.core_count = cpu.core_count;
        m.latest_cpu_metrics.current = cpu.current;
        m.latest_cpu_metrics.load_avg_1m = cpu.load_avg_1m;
        m.latest_cpu_metrics.load_avg_5m = cpu.load_avg_5m;
        m.latest_cpu_metrics.load_avg_15m = cpu.load_avg_15m;
        m.latest_cpu_metrics.power = cpu.power;
        m.latest_cpu_metrics.temperature = cpu.temperature;
        m.latest_cpu_metrics.usage_percent = cpu.usage_percent;
        m.latest_cpu_metrics.voltage = cpu.voltage;
        m.latest_cpu_metrics.timestamp = now_seconds;

        // Memory
        const auto& mem = res->resource.memory;
        m.latest_memory_metrics.total = mem.total;
        m.latest_memory_metrics.used = mem.used;
        m.latest_memory_metrics.free = mem.free;
        m.latest_memory_metrics.usage_percent = mem.usage_percent;
        m.latest_memory_metrics.timestamp = now_seconds;

        // Network
        m.latest_network_metrics.network_count = (int)res->resource.network.size();
        m.latest_network_metrics.networks.clear();
        m.latest_network_metrics.networks.reserve(res->resource.network.size());
        for (const auto& ni : res->resource.network) {
            NetworkSample s;
            s.interface = ni.interface;
            s.rx_bytes = ni.rx_bytes; s.tx_bytes = ni.tx_bytes;
            s.rx_packets = ni.rx_packets; s.tx_packets = ni.tx_packets;
            s.rx_errors = ni.rx_errors; s.tx_errors = ni.tx_errors;
            s.rx_rate = ni.rx_rate; s.tx_rate = ni.tx_rate;
            s.rx_drop_rate = ni.rx_drop_rate; s.tx_drop_rate = ni.tx_drop_rate;
            s.state = ni.state;
            s.timestamp = now_seconds;
            m.latest_network_metrics.networks.push_back(std::move(s));
        }
        m.latest_network_metrics.timestamp = now_seconds;

        // Disk
        m.latest_disk_metrics.disk_count = (int)res->resource.disk.size();
        m.latest_disk_metrics.disks.clear();
        m.latest_disk_metrics.disks.reserve(res->resource.disk.size());
        for (const auto& dk : res->resource.disk) {
            DiskSample d;
            d.device = dk.device;
            d.mount_point = dk.mount_point;
            d.total = dk.total; d.used = dk.used; d.free = dk.free;
            d.usage_percent = dk.usage_percent;
            d.timestamp = now_seconds;
            m.latest_disk_metrics.disks.push_back(std::move(d));
        }
        m.latest_disk_metrics.timestamp = now_seconds;

        // GPU
        m.latest_gpu_metrics.gpu_count = (int)res->resource.gpu.size();
        m.latest_gpu_metrics.gpus.clear();
        m.latest_gpu_metrics.gpus.reserve(res->resource.gpu.size());
        for (const auto& g : res->resource.gpu) {
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
            gs.timestamp = now_seconds;
            m.latest_gpu_metrics.gpus.push_back(std::move(gs));
        }
        m.latest_gpu_metrics.timestamp = now_seconds;
    }

    // BMC传感器数据（如果提供）
    if (bmc_sensors && !bmc_sensors->empty()) {
        m.latest_sensor_metrics.sensor_count = static_cast<int>(bmc_sensors->size());
        m.latest_sensor_metrics.sensors.clear();
        m.latest_sensor_metrics.sensors.reserve(bmc_sensors->size());
        
        for (const auto& [sensorname, sensor_row] : *bmc_sensors) {
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
            m.latest_sensor_metrics.sensors.push_back(std::move(sensor_json));
        }
        m.latest_sensor_metrics.timestamp = now_seconds;
    } else {
        // 传感器（暂无数据，保持空）
        m.latest_sensor_metrics.sensor_count = 0;
        m.latest_sensor_metrics.sensors.clear();
        m.latest_sensor_metrics.timestamp = now_seconds;
    }

    return m;
}

// inline 定义在头文件中
} // namespace mapper
} // namespace web
} // namespace yw




