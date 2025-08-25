#include "web_controller.h"
#include <hv/HttpService.h>
#include "yw/node.h"
#include "yw/monitor.h"
#include "yw/bmc.h"
#include "yw/bmc_model.h"
#include "web_model.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace yw {
namespace web {

using json = nlohmann::json;

WebController::WebController(std::shared_ptr<hv::HttpService> service,
                             std::shared_ptr<node::INodeModule> node_module,
                             std::shared_ptr<monitor::IMonitorModule> monitor_module,
                             std::shared_ptr<bmc::IBMCModule> bmc_module)
    : service_(std::move(service)),
      node_module_(std::move(node_module)),
      monitor_module_(std::move(monitor_module)),
      bmc_module_(std::move(bmc_module)) {
    if (service_) {
        service_->AllowCORS();
        setupRoutes();
    }
}

WebController::~WebController() = default;

void WebController::setupRoutes() {

    // 汇总所有节点的 NodeMetrics（由 NodeExt + 最新 Resource 拼装）
    service_->GET("/node/metrics", [this](const HttpContextPtr& ctx) {
        if (!node_module_) return 500;
        const auto now_ms = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::system_clock::now()
        ).time_since_epoch().count();

        auto nodes = node_module_->getAllNodes();
        std::vector<NodeMetrics> result;
        result.reserve(nodes.size());

        for (const auto& nx : nodes) {
            NodeMetrics m;
            // NodeExt 基础信息
            m.box_id = nx.box_id;
            m.cpu_id = nx.cpu_id;
            m.slot_id = nx.slot_id;
            m.host_ip = nx.host_ip;
            m.status = nx.status;
            m.updated_at = nx.updated_at;

            // 资源快照（若存在）
            if (monitor_module_) {
                auto resPtr = monitor_module_->getNodeResource(nx.host_ip);
                if (resPtr) {
                    // 组件
                    m.component = resPtr->component;
                    // 容器统计
                    int running = 0, paused = 0, stopped = 0;
                    for (const auto& c : resPtr->component) {
                        if (c.state == "RUNNING") running++;
                        else if (c.state == "PAUSED") paused++;
                        else if (c.state == "STOPPED") stopped++;
                    }
                    m.latest_container_metrics.container_count = (int)resPtr->component.size();
                    m.latest_container_metrics.running_count = running;
                    m.latest_container_metrics.paused_count = paused;
                    m.latest_container_metrics.stopped_count = stopped;
                    m.latest_container_metrics.timestamp = now_ms;

                    // CPU
                    const auto& cpu = resPtr->resource.cpu;
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
                    m.latest_cpu_metrics.timestamp = now_ms;

                    // Memory
                    const auto& mem = resPtr->resource.memory;
                    m.latest_memory_metrics.total = mem.total;
                    m.latest_memory_metrics.used = mem.used;
                    m.latest_memory_metrics.free = mem.free;
                    m.latest_memory_metrics.usage_percent = mem.usage_percent;
                    m.latest_memory_metrics.timestamp = now_ms;

                    // Network
                    m.latest_network_metrics.network_count = (int)resPtr->resource.network.size();
                    m.latest_network_metrics.networks.clear();
                    m.latest_network_metrics.networks.reserve(resPtr->resource.network.size());
                    for (const auto& ni : resPtr->resource.network) {
                        NetworkSample s;
                        s.interface = ni.interface;
                        s.rx_bytes = ni.rx_bytes; s.tx_bytes = ni.tx_bytes;
                        s.rx_packets = ni.rx_packets; s.tx_packets = ni.tx_packets;
                        s.rx_errors = ni.rx_errors; s.tx_errors = ni.tx_errors;
                        s.rx_rate = ni.rx_rate; s.tx_rate = ni.tx_rate;
                        s.timestamp = now_ms;
                        m.latest_network_metrics.networks.push_back(std::move(s));
                    }
                    m.latest_network_metrics.timestamp = now_ms;

                    // Disk
                    m.latest_disk_metrics.disk_count = (int)resPtr->resource.disk.size();
                    m.latest_disk_metrics.disks.clear();
                    m.latest_disk_metrics.disks.reserve(resPtr->resource.disk.size());
                    for (const auto& dk : resPtr->resource.disk) {
                        DiskSample d;
                        d.device = dk.device;
                        d.mount_point = dk.mount_point;
                        d.total = dk.total; d.used = dk.used; d.free = dk.free;
                        d.usage_percent = dk.usage_percent;
                        d.timestamp = now_ms;
                        m.latest_disk_metrics.disks.push_back(std::move(d));
                    }
                    m.latest_disk_metrics.timestamp = now_ms;

                    // GPU（无电流电压字段，置 0）
                    m.latest_gpu_metrics.gpu_count = (int)resPtr->resource.gpu.size();
                    m.latest_gpu_metrics.gpus.clear();
                    m.latest_gpu_metrics.gpus.reserve(resPtr->resource.gpu.size());
                    for (const auto& g : resPtr->resource.gpu) {
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
                        gs.timestamp = now_ms;
                        m.latest_gpu_metrics.gpus.push_back(std::move(gs));
                    }
                    m.latest_gpu_metrics.timestamp = now_ms;

                    // 传感器（暂无结构，保持空）
                    m.latest_sensor_metrics.sensor_count = 0;
                    m.latest_sensor_metrics.sensors.clear();
                    m.latest_sensor_metrics.timestamp = now_ms;
                }
            }

            result.push_back(std::move(m));
        }

        json resp = {
            {"api_version", 1},
            {"data", {
                {"nodes_metrics", result}
            }},
            {"status", "success"},
        };
        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // 查询近一段时间的资源序列（透传到 IMonitorModule）
    service_->GET("/node/historical-metrics", [this](const HttpContextPtr& ctx) {
        if (!monitor_module_) return 500;

        std::string ip;
        std::string duration = "1m"; // 简写，后续由模块转换
        std::vector<std::string> kinds;

        auto params = ctx->params();
        if (params.find("host_ip") != params.end()) ip = params["host_ip"];
        if (params.find("time_range") != params.end()) duration = params["time_range"];
        if (params.find("metrics") != params.end()) {
            const std::string& ks = params["metrics"]; // e.g. cpu,memory,network
            std::string item;
            for (size_t i = 0, n = ks.size(); i <= n; ++i) {
                if (i == n || ks[i] == ',') {
                    if (!item.empty()) kinds.push_back(item);
                    item.clear();
                } else {
                    item.push_back(ks[i]);
                }
            }
        }

        if (ip.empty()) {
            return ctx->send("{\"error\":\"missing ip\"}");
        }

        try {
            auto series = monitor_module_->queryMetricsSeries(ip, duration, kinds);
            monitor::ResourceWindow win;
            win.host_ip = ip;
            win.metrics = std::move(series);
            win.time_range = duration;
            if (node_module_) {
                auto nodeOpt = node_module_->getNodeByIP(ip);
                if (nodeOpt) {
                    win.box_id = nodeOpt->box_id;
                    win.cpu_id = nodeOpt->cpu_id;
                    win.slot_id = nodeOpt->slot_id;
                }
            }

            nlohmann::json resp_historical_metrics = win;

            // 最小改动：在响应中并入 BMC 传感器数据为并列字段 bmc_sensors
            if (bmc_module_) {
                auto grouped = bmc_module_->queryBMCSensor(ip, duration);
                nlohmann::json bmc_json = grouped; // 直接序列化 map<string, vector<BMCSensorRow>>
                resp_historical_metrics["metrics"]["sensor"] = std::move(bmc_json);
            }

            nlohmann::json resp = {
                {"api_version", 1},
                {"data", {
                    {"historical_metrics", resp_historical_metrics}
                }},
                {"status", "success"},
            };

            ctx->setContentType("application/json");
            return ctx->send(resp.dump(2));
        } catch (const std::exception& e) {
            std::cerr << "query resource failed: " << e.what() << std::endl;
            return 500;
        }
    });

    // 获取所有 box 的最新 BMC 简要信息
    service_->GET("/box/bmc", [this](const HttpContextPtr& ctx) {
        if (!bmc_module_) return 500;

        auto all = bmc_module_->getAllBoxBMC();
        nlohmann::json arr = nlohmann::json::array();
        arr.get_ref<nlohmann::json::array_t&>().reserve(all.size());
        for (const auto& info : all) {
            nlohmann::json j;
            j["box_id"] = info.boxid;
            j["box_name"] = info.boxname;
            j["timestamp"] = info.timestamp;
            // fan data
            nlohmann::json fans = nlohmann::json::array();
            fans.get_ref<nlohmann::json::array_t&>().reserve(2);
            for (int i = 0; i < 2; ++i) {
                const auto& f = info.fan[i];
                nlohmann::json jf;
                jf["fanseq"] = f.fanseq;
                jf["fanmode"] = f.fanmode;
                jf["fanspeed"] = f.fanspeed;

                // 解码：报警类型/工作模式（各4位）
                std::uint8_t mode = f.fanmode;
                int alarm_type = (mode >> 4) & 0x0F; // 高4位
                int work_mode = (mode & 0x0F);       // 低4位（0自动，1手动）
                jf["alarm_type"] = alarm_type;
                jf["work_mode"] = work_mode;

                // 解码：转速（高1位为单位，后7位为数值）
                std::uint8_t speed_byte = static_cast<std::uint8_t>(f.fanspeed & 0xFF);
                int speed_unit = (speed_byte >> 7) & 0x01; // 0等级 1占空比
                int speed_val = (speed_byte & 0x7F);
                if (speed_unit == 0) {
                    jf["speed_unit"] = "level";   // 1低速 2中速 3高速
                    jf["speed_level"] = speed_val;
                } else {
                    jf["speed_unit"] = "duty";    // 占空比百分比 1-100
                    jf["duty_cycle"] = speed_val;
                }
                fans.push_back(std::move(jf));
            }
            j["fan"] = std::move(fans);
            arr.push_back(std::move(j));
        }

        nlohmann::json resp = {
            {"api_version", 1},
            {"data", {
                {"box_bmc", arr}
            }},
            {"status", "success"},
        };

        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });

    // 获取所有节点（由 INodeModule 提供数据），并附带组件列表（由 IMonitorModule 提供）
    service_->GET("/node", [this](const HttpContextPtr& ctx) {
        if (!node_module_) return 500;
        const auto nodes = node_module_->getAllNodes();
        int filter_box_id = -1;
        bool has_filter = false;
        std::string filter_host_ip;
        bool has_host_filter = false;
        auto params = ctx->params();
        if (params.find("box_id") != params.end()) {
            try {
                filter_box_id = std::stoi(params["box_id"]);
                has_filter = true;
            } catch (...) {
                ctx->setContentType("application/json");
                return ctx->send("{\"error\":\"invalid box_id\"}");
            }
        }
        if (params.find("host_ip") != params.end()) {
            filter_host_ip = params["host_ip"];
            has_host_filter = true;
        }
        nlohmann::json resp_nodes = nlohmann::json::array();
        resp_nodes.get_ref<nlohmann::json::array_t&>().reserve(nodes.size());
        for (const auto& ext : nodes) {
            if (has_filter && ext.box_id != filter_box_id) continue;
            if (has_host_filter && ext.host_ip != filter_host_ip) continue;
            nlohmann::json j = ext;
            if (monitor_module_) {
                auto resPtr = monitor_module_->getNodeResource(ext.host_ip);
                if (resPtr) {
                    j["component"] = resPtr->component;
                } else {
                    j["component"] = nlohmann::json::array();
                }
            }
            resp_nodes.push_back(std::move(j));
        }

        nlohmann::json resp;
        if (has_host_filter) {
            resp = {
                {"api_version", 1},
                {"data", resp_nodes[0]},
                {"status", "success"},
            };

        } else {
            resp = {
                {"api_version", 1},
                {"data", 
                    {"nodes", resp_nodes}
                },
                {"status", "success"},
            };
        }

        ctx->setContentType("application/json");
        return ctx->send(resp.dump(2));
    });
}

} // namespace web
} // namespace yw


