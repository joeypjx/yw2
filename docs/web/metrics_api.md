# 监控指标接口文档

本文档说明节点监控指标相关的HTTP API接口。

## 接口概览

监控指标接口提供节点监控数据的查询功能，包括：
- 节点最新指标查询
- 节点历史指标查询

## 接口列表

### 1. 获取所有节点的最新指标数据

**接口路径：** `GET /node/metrics`

**功能描述：** 获取所有节点的最新监控指标数据，包括CPU、内存、磁盘、网络、GPU、BMC传感器等指标。

**查询参数：** 无

**请求示例：**

```bash
GET /node/metrics
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes_metrics": [
      {
        "bmc_company": 0,
        "bmc_version": "",
        "board_type": "",
        "box_id": 0,
        "box_type": "",
        "component": [],
        "cpu_arch": "",
        "cpu_id": 1,
        "cpu_type": "",
        "host_ip": "192.168.10.58",
        "hostname": "",
        "id": 0,
        "ipmb_address": 0,
        "module_type": 0,
        "os_type": "",
        "resource_type": "",
        "service_port": 0,
        "slot_id": 0,
        "srio_id": 0,
        "status": "online",
        "updated_at": 1761211844161,
        "latest_cpu_metrics": {
          "timestamp": 1761211845,
          "usage_percent": 64.98316498316498,
          "load_avg_1m": 10.32,
          "load_avg_5m": 9.93,
          "load_avg_15m": 7.62,
          "core_count": 8,
          "core_allocated": 0,
          "temperature": 0.0,
          "voltage": 0.0,
          "current": 0.0,
          "power": 0.0
        },
        "latest_memory_metrics": {
          "timestamp": 1761211845,
          "total": 15645540352,
          "used": 7531069440,
          "free": 8114470912,
          "usage_percent": 48.13556624164335
        },
        "latest_disk_metrics": {
          "timestamp": 1761211845,
          "disk_count": 2,
          "disks": [
            {
              "device": "/dev/sda4",
              "mount_point": "/",
              "total": 75125227520,
              "used": 28161150976,
              "free": 46964076544,
              "usage_percent": 37.48561156570591
            },
            {
              "device": "/dev/sda5",
              "mount_point": "/data",
              "total": 426474012672,
              "used": 74554384384,
              "free": 351919628288,
              "usage_percent": 17.481577345567263
            }
          ]
        },
        "latest_network_metrics": {
          "timestamp": 1761211845,
          "network_count": 2,
          "networks": [
            {
              "interface": "enaphyt4i0",
              "rx_bytes": 9188401652,
              "tx_bytes": 1978095272,
              "rx_packets": 13810021,
              "tx_packets": 3260419,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 16058.0,
              "tx_rate": 8690.0,
              "rx_drop_rate": 0.0,
              "tx_drop_rate": 0.0,
              "state": 0
            },
            {
              "interface": "docker0",
              "rx_bytes": 59223458,
              "tx_bytes": 110288783,
              "rx_packets": 476990,
              "tx_packets": 579150,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 2812.0,
              "tx_rate": 3074.0,
              "rx_drop_rate": 0.0,
              "tx_drop_rate": 0.0,
              "state": 0
            }
          ]
        },
        "latest_gpu_metrics": {
          "timestamp": 1761211845,
          "gpu_count": 1,
          "gpus": [
            {
              "index": 0,
              "name": " Iluvatar MR-V50A",
              "compute_usage": 0.0,
              "mem_total": 17179869184,
              "mem_used": 119537664,
              "mem_usage": 1.0,
              "temperature": 45.0,
              "power": 20.0,
              "voltage": 0.0,
              "current": 0.0
            }
          ]
        },
        "latest_sensor_metrics": {
          "timestamp": 1761211845,
          "sensor_count": 2,
          "sensors": [
            {
              "timestamp": 1761211840,
              "host_ip": "192.168.10.58",
              "sensorseq": 1,
              "sensortype": 1,
              "sensorname": "Temperature_CPU",
              "sensorvalue_L": 50,
              "sensorvalue_H": 65,
              "sensor_value": 65.5,
              "sensoralmtype": 0
            },
            {
              "timestamp": 1761211840,
              "host_ip": "192.168.10.58",
              "sensorseq": 2,
              "sensortype": 2,
              "sensorname": "Voltage_12V",
              "sensorvalue_L": 0,
              "sensorvalue_H": 12,
              "sensor_value": 12.0,
              "sensoralmtype": 0
            }
          ]
        },
        "latest_container_metrics": {
          "timestamp": 1761211845,
          "container_count": 0,
          "running_count": 0,
          "stopped_count": 0,
          "paused_count": 0
        }
      }
    ]
  }
}
```

**注意：** 如果部分节点处理失败，响应中会包含 `warnings` 字段：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes_metrics": [...],
    "warnings": {
      "failed_nodes_count": 2
    }
  }
}
```

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| bmc_company | integer | BMC公司标识 |
| bmc_version | string | BMC版本 |
| board_type | string | 板卡类型 |
| box_id | integer | 机箱编号 |
| box_type | string | 机箱类型 |
| component | array | 组件列表 |
| cpu_arch | string | CPU架构 |
| cpu_id | integer | CPU ID |
| cpu_type | string | CPU类型 |
| host_ip | string | 节点IP地址 |
| hostname | string | 节点主机名 |
| id | integer | 节点ID |
| ipmb_address | integer | IPMB地址 |
| module_type | integer | 模块类型 |
| os_type | string | 操作系统类型 |
| resource_type | string | 资源类型 |
| service_port | integer | 服务端口 |
| slot_id | integer | 槽位号 |
| srio_id | integer | SRIO ID |
| status | string | 节点状态（online/offline） |
| updated_at | integer | 最后更新时间（毫秒时间戳） |
| warnings | object | 警告信息（可选，仅在部分节点处理失败时出现） |
| warnings.failed_nodes_count | integer | 处理失败的节点数量 |
| latest_cpu_metrics | object | 最新CPU指标 |
| latest_cpu_metrics.timestamp | integer | 时间戳（秒级） |
| latest_cpu_metrics.usage_percent | float | CPU使用率百分比 |
| latest_cpu_metrics.load_avg_1m | float | 1分钟平均负载 |
| latest_cpu_metrics.load_avg_5m | float | 5分钟平均负载 |
| latest_cpu_metrics.load_avg_15m | float | 15分钟平均负载 |
| latest_cpu_metrics.core_count | integer | CPU核心总数 |
| latest_cpu_metrics.core_allocated | integer | 已分配核心数 |
| latest_cpu_metrics.temperature | float | CPU温度 |
| latest_cpu_metrics.voltage | float | CPU电压 |
| latest_cpu_metrics.current | float | CPU电流 |
| latest_cpu_metrics.power | float | CPU功耗 |
| latest_memory_metrics | object | 最新内存指标 |
| latest_memory_metrics.timestamp | integer | 时间戳（秒级） |
| latest_memory_metrics.total | integer | 总内存（字节） |
| latest_memory_metrics.used | integer | 已使用内存（字节） |
| latest_memory_metrics.free | integer | 空闲内存（字节） |
| latest_memory_metrics.usage_percent | float | 内存使用率百分比 |
| latest_disk_metrics | object | 最新磁盘指标 |
| latest_disk_metrics.timestamp | integer | 时间戳（秒级） |
| latest_disk_metrics.disk_count | integer | 磁盘数量 |
| latest_disk_metrics.disks | array | 磁盘列表 |
| latest_disk_metrics.disks[].device | string | 设备名 |
| latest_disk_metrics.disks[].mount_point | string | 挂载点 |
| latest_disk_metrics.disks[].total | integer | 总容量（字节） |
| latest_disk_metrics.disks[].used | integer | 已使用（字节） |
| latest_disk_metrics.disks[].free | integer | 空闲（字节） |
| latest_disk_metrics.disks[].usage_percent | float | 使用率百分比 |
| latest_network_metrics | object | 最新网络指标 |
| latest_network_metrics.timestamp | integer | 时间戳（秒级） |
| latest_network_metrics.network_count | integer | 网络接口数量 |
| latest_network_metrics.networks | array | 网络接口列表 |
| latest_network_metrics.networks[].interface | string | 网络接口名 |
| latest_network_metrics.networks[].rx_bytes | integer | 接收字节数 |
| latest_network_metrics.networks[].tx_bytes | integer | 发送字节数 |
| latest_network_metrics.networks[].rx_packets | integer | 接收包数 |
| latest_network_metrics.networks[].tx_packets | integer | 发送包数 |
| latest_network_metrics.networks[].rx_errors | integer | 接收错误数 |
| latest_network_metrics.networks[].tx_errors | integer | 发送错误数 |
| latest_network_metrics.networks[].rx_rate | float | 接收速率（字节/秒） |
| latest_network_metrics.networks[].tx_rate | float | 发送速率（字节/秒） |
| latest_network_metrics.networks[].rx_drop_rate | float | 接收丢包速率（字节/秒，可选，通常为0，实际响应中可能不出现） |
| latest_network_metrics.networks[].tx_drop_rate | float | 发送丢包速率（字节/秒，可选，通常为0，实际响应中可能不出现） |
| latest_network_metrics.networks[].state | integer | 网络接口状态（可选，通常为0，实际响应中可能不出现） |
| latest_gpu_metrics | object | 最新GPU指标 |
| latest_gpu_metrics.timestamp | integer | 时间戳（秒级） |
| latest_gpu_metrics.gpu_count | integer | GPU数量 |
| latest_gpu_metrics.gpus | array | GPU列表 |
| latest_gpu_metrics.gpus[].index | integer | GPU索引 |
| latest_gpu_metrics.gpus[].name | string | GPU名称 |
| latest_gpu_metrics.gpus[].compute_usage | float | 计算使用率百分比 |
| latest_gpu_metrics.gpus[].mem_total | integer | GPU总内存（字节） |
| latest_gpu_metrics.gpus[].mem_used | integer | GPU已使用内存（字节） |
| latest_gpu_metrics.gpus[].mem_usage | float | GPU内存使用率百分比 |
| latest_gpu_metrics.gpus[].temperature | float | GPU温度 |
| latest_gpu_metrics.gpus[].power | float | GPU功耗 |
| latest_gpu_metrics.gpus[].voltage | float | GPU电压 |
| latest_gpu_metrics.gpus[].current | float | GPU电流 |
| latest_sensor_metrics | object | 最新传感器指标（BMC传感器数据） |
| latest_sensor_metrics.timestamp | integer | 时间戳（秒级） |
| latest_sensor_metrics.sensor_count | integer | 传感器数量 |
| latest_sensor_metrics.sensors | array | 传感器列表，每个元素是一个传感器JSON对象 |
| latest_sensor_metrics.sensors[].timestamp | integer | 传感器数据时间戳（秒级） |
| latest_sensor_metrics.sensors[].host_ip | string | 节点IP地址 |
| latest_sensor_metrics.sensors[].sensorseq | integer | 传感器序号 |
| latest_sensor_metrics.sensors[].sensortype | integer | 传感器类型 |
| latest_sensor_metrics.sensors[].sensorname | string | 传感器名称 |
| latest_sensor_metrics.sensors[].sensorvalue_L | integer | 传感器数值低字节（小数部分） |
| latest_sensor_metrics.sensors[].sensorvalue_H | integer | 传感器数值高字节（整数部分） |
| latest_sensor_metrics.sensors[].sensor_value | float | 传感器实际数值（sensorvalue_H + sensorvalue_L * 0.01） |
| latest_sensor_metrics.sensors[].sensoralmtype | integer | 告警类型 |
| latest_container_metrics | object | 最新容器指标 |
| latest_container_metrics.timestamp | integer | 时间戳（秒级） |
| latest_container_metrics.container_count | integer | 容器总数 |
| latest_container_metrics.running_count | integer | 运行中容器数 |
| latest_container_metrics.stopped_count | integer | 已停止容器数 |
| latest_container_metrics.paused_count | integer | 暂停容器数 |

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 2. 查询节点历史指标数据

**接口路径：** `GET /node/historical-metrics`

**功能描述：** 查询指定节点在指定时间范围内的历史监控指标时序数据。

**查询参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| host_ip | string | 是 | 节点IP地址 |
| time_range | string | 否 | 时间范围，默认"1m"，支持格式：数字+单位（如"5m"、"1h"、"30s"） |
| metrics | string | 否 | 指标类型，多个用逗号分隔（如"cpu,memory"），可选值：cpu、memory、network、disk、gpu，空值表示查询全部类型 |

**请求示例：**

```bash
# 查询最近1分钟的所有指标数据
GET /node/historical-metrics?host_ip=192.168.10.58

# 查询最近5分钟的CPU和内存数据
GET /node/historical-metrics?host_ip=192.168.10.58&time_range=5m&metrics=cpu,memory
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "historical_metrics": {
      "host_ip": "192.168.10.58",
      "box_id": 0,
      "cpu_id": 1,
      "slot_id": 0,
      "time_range": "5m",
      "metrics": {
        "cpu": [
          {
            "timestamp": 1761211690,
            "usage_percent": 79.65936284595003,
            "load_avg_1m": 8.952222222222224,
            "load_avg_5m": 9.33222222222222,
            "load_avg_15m": 7.046666666666668,
            "core_count": 8,
            "core_allocated": 2,
            "temperature": 0.0,
            "voltage": 0.0,
            "current": 0.0,
            "power": 0.0
          }
        ],
        "memory": [
          {
            "timestamp": 1761211690,
            "total": 15645540352,
            "used": 7426095332,
            "free": 8219445020,
            "usage_percent": 47.46461396973268
          }
        ],
        "disk": {
          "/dev/sda4": [
            {
              "timestamp": 1761211690,
              "device": "/dev/sda4",
              "mount_point": "/",
              "total": 75125227520,
              "used": 28161017173,
              "free": 46964210347,
              "usage_percent": 37.48543345953428
            }
          ],
          "/dev/sda5": [
            {
              "timestamp": 1761211690,
              "device": "/dev/sda5",
              "mount_point": "/data",
              "total": 426474012672,
              "used": 74547931364,
              "free": 351926081308,
              "usage_percent": 17.480064235681848
            }
          ]
        },
        "network": {
          "docker0": [
            {
              "timestamp": 1761211690,
              "interface": "docker0",
              "rx_bytes": 58655610,
              "tx_bytes": 109164219,
              "rx_packets": 472325,
              "tx_packets": 573636,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 3083,
              "tx_rate": 5481
            }
          ],
          "enaphyt4i0": [
            {
              "timestamp": 1761211690,
              "interface": "enaphyt4i0",
              "rx_bytes": 9186164636,
              "tx_bytes": 1976766915,
              "rx_packets": 13777094,
              "tx_packets": 3253110,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 16807.0,
              "tx_rate": 10569.0,
              "rx_drop_rate": 0.0,
              "tx_drop_rate": 0.0,
              "state": 0
            }
          ]
        },
        "gpu": {
          "gpu_0": [
            {
              "timestamp": 1761211690,
              "index": 0,
              "name": " Iluvatar MR-V50A",
              "compute_usage": 0.0,
              "mem_total": 17179869184,
              "mem_used": 119537664,
              "mem_usage": 1.0,
              "temperature": 44.55555555555556,
              "power": 19.77777777777778,
              "free": 0
            }
          ]
        },
        "sensor": {
          "Temperature_CPU": [
            {
              "timestamp": 1761211690,
              "host_ip": "192.168.10.58",
              "sensorseq": 1,
              "sensortype": 1,
              "sensorname": "Temperature_CPU",
              "sensorvalue_L": 50,
              "sensorvalue_H": 65,
              "sensor_value": 65.5,
              "sensoralmtype": 0
            },
            {
              "timestamp": 1761211700,
              "host_ip": "192.168.10.58",
              "sensorseq": 1,
              "sensortype": 1,
              "sensorname": "Temperature_CPU",
              "sensorvalue_L": 60,
              "sensorvalue_H": 66,
              "sensor_value": 66.6,
              "sensoralmtype": 0
            }
          ],
          "Voltage_12V": [
            {
              "timestamp": 1761211690,
              "host_ip": "192.168.10.58",
              "sensorseq": 2,
              "sensortype": 2,
              "sensorname": "Voltage_12V",
              "sensorvalue_L": 0,
              "sensorvalue_H": 12,
              "sensor_value": 12.0,
              "sensoralmtype": 0
            }
          ]
        }
      }
    }
  }
}
```

**注意：** 如果没有传感器数据，`sensor` 字段为空对象 `{}`。

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| host_ip | string | 节点IP地址 |
| box_id | integer | 机箱编号 |
| cpu_id | integer | CPU ID |
| slot_id | integer | 槽位号 |
| time_range | string | 时间范围 |
| metrics | object | 指标数据对象 |
| metrics.cpu | array | CPU时序数据数组，每个元素包含一个时间点的所有CPU指标 |
| metrics.cpu[].timestamp | integer | 时间戳（秒级） |
| metrics.cpu[].usage_percent | float | CPU使用率百分比 |
| metrics.cpu[].load_avg_1m | float | 1分钟平均负载 |
| metrics.cpu[].load_avg_5m | float | 5分钟平均负载 |
| metrics.cpu[].load_avg_15m | float | 15分钟平均负载 |
| metrics.cpu[].core_count | integer | CPU核心总数 |
| metrics.cpu[].core_allocated | integer | 已分配核心数 |
| metrics.cpu[].temperature | float | CPU温度 |
| metrics.cpu[].voltage | float | CPU电压 |
| metrics.cpu[].current | float | CPU电流 |
| metrics.cpu[].power | float | CPU功耗 |
| metrics.memory | array | 内存时序数据数组，每个元素包含一个时间点的所有内存指标 |
| metrics.memory[].timestamp | integer | 时间戳（秒级） |
| metrics.memory[].total | integer | 总内存（字节） |
| metrics.memory[].used | integer | 已使用内存（字节） |
| metrics.memory[].free | integer | 空闲内存（字节） |
| metrics.memory[].usage_percent | float | 内存使用率百分比 |
| metrics.disk | object | 磁盘时序数据（按设备名索引，如"/dev/sda4"） |
| metrics.disk.{device} | array | 指定设备的时序数据数组 |
| metrics.disk.{device}[].timestamp | integer | 时间戳（秒级） |
| metrics.disk.{device}[].device | string | 设备名 |
| metrics.disk.{device}[].mount_point | string | 挂载点 |
| metrics.disk.{device}[].total | integer | 总容量（字节） |
| metrics.disk.{device}[].used | integer | 已使用（字节） |
| metrics.disk.{device}[].free | integer | 空闲（字节） |
| metrics.disk.{device}[].usage_percent | float | 使用率百分比 |
| metrics.network | object | 网络时序数据（按接口名索引，如"docker0"、"enaphyt4i0"） |
| metrics.network.{interface} | array | 指定接口的时序数据数组 |
| metrics.network.{interface}[].timestamp | integer | 时间戳（秒级） |
| metrics.network.{interface}[].interface | string | 网络接口名 |
| metrics.network.{interface}[].rx_bytes | integer | 接收字节数 |
| metrics.network.{interface}[].tx_bytes | integer | 发送字节数 |
| metrics.network.{interface}[].rx_packets | integer | 接收包数 |
| metrics.network.{interface}[].tx_packets | integer | 发送包数 |
| metrics.network.{interface}[].rx_errors | integer | 接收错误数 |
| metrics.network.{interface}[].tx_errors | integer | 发送错误数 |
| metrics.network.{interface}[].rx_rate | float | 接收速率（字节/秒） |
| metrics.network.{interface}[].tx_rate | float | 发送速率（字节/秒） |
| metrics.network.{interface}[].rx_drop_rate | float | 接收丢包速率（字节/秒，通常为0） |
| metrics.network.{interface}[].tx_drop_rate | float | 发送丢包速率（字节/秒，通常为0） |
| metrics.network.{interface}[].state | integer | 网络接口状态（通常为0） |
| metrics.gpu | object | GPU时序数据（按GPU索引索引，如"gpu_0"） |
| metrics.gpu.{gpu_index} | array | 指定GPU的时序数据数组 |
| metrics.gpu.{gpu_index}[].timestamp | integer | 时间戳（秒级） |
| metrics.gpu.{gpu_index}[].index | integer | GPU索引 |
| metrics.gpu.{gpu_index}[].name | string | GPU名称 |
| metrics.gpu.{gpu_index}[].compute_usage | float | GPU计算使用率百分比 |
| metrics.gpu.{gpu_index}[].mem_total | integer | GPU总内存（字节） |
| metrics.gpu.{gpu_index}[].mem_used | integer | GPU已使用内存（字节） |
| metrics.gpu.{gpu_index}[].mem_usage | float | GPU内存使用率百分比 |
| metrics.gpu.{gpu_index}[].temperature | float | GPU温度 |
| metrics.gpu.{gpu_index}[].power | float | GPU功耗 |
| metrics.gpu.{gpu_index}[].free | integer | GPU空闲状态（通常为0） |
| metrics.sensor | object | BMC传感器时序数据（按传感器名称索引） |
| metrics.sensor.{sensor_name} | array | 指定传感器的时序数据数组 |
| metrics.sensor.{sensor_name}[].timestamp | integer | 时间戳（秒级） |
| metrics.sensor.{sensor_name}[].host_ip | string | 节点IP地址 |
| metrics.sensor.{sensor_name}[].sensorseq | integer | 传感器序号 |
| metrics.sensor.{sensor_name}[].sensortype | integer | 传感器类型 |
| metrics.sensor.{sensor_name}[].sensorname | string | 传感器名称 |
| metrics.sensor.{sensor_name}[].sensorvalue_L | integer | 传感器数值低字节（小数部分） |
| metrics.sensor.{sensor_name}[].sensorvalue_H | integer | 传感器数值高字节（整数部分） |
| metrics.sensor.{sensor_name}[].sensor_value | float | 传感器数值（sensorvalue_H + sensorvalue_L * 0.01） |
| metrics.sensor.{sensor_name}[].sensoralmtype | integer | 告警类型 |

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

## 注意事项

1. 最新指标数据从数据库查询节点最近一次的资源快照
2. 历史指标数据支持按时间范围查询，时间范围格式支持：
   - 简写格式：数字+单位（如"1h"、"5m"、"30s"）
   - PostgreSQL interval格式（如"1 hour"、"5 minutes"）
3. 指标类型参数为空时，查询所有类型的指标数据
4. 时序数据中每个指标类型都是数组，数组中的每个元素代表一个时间点的完整指标数据
5. 磁盘数据按设备名（device）索引，网络数据按接口名（interface）索引，GPU数据按GPU索引（gpu_0、gpu_1等）索引
6. 历史指标数据中，网络数据的 `rx_rate` 和 `tx_rate` 字段类型为 `float`，不是 `integer`
7. 历史指标数据中，GPU数据不包含 `voltage`、`current` 字段，但包含 `free` 字段（通常为0）
8. 历史指标数据中，网络数据包含 `rx_drop_rate`、`tx_drop_rate`、`state` 字段（通常为0），但实际响应中可能不出现（当值为0时）
9. 历史指标数据中，`sensor` 字段是一个对象，按传感器名称（sensorname）索引，每个传感器对应一个时序数据数组
10. `sensor` 字段中的每个数据点包含完整的传感器信息，包括时间戳、传感器序号、类型、名称、数值（分为高字节和低字节）以及计算后的实际值（sensor_value）
11. 如果没有传感器数据或BMC模块不可用，`sensor` 字段为空对象 `{}`
12. `/node/metrics` 接口中的 `latest_sensor_metrics` 字段**可以有内容**，前提是：
    - BMC模块可用
    - 节点有BMC传感器数据（数据库中最近5分钟内有数据）
    - 查询成功
    - 如果以上条件不满足，`sensor_count` 为 0，`sensors` 为空数组
13. 如果部分节点处理失败，响应中会包含 `warnings` 字段，显示失败节点数量
