# API 接口数据格式文档

本文档详细描述了系统中所有 API 接口的请求和响应数据格式。

## 目录

- [通用响应格式](#通用响应格式)
- [告警接口](#告警接口)
- [BMC 接口](#bmc-接口)
- [节点接口](#节点接口)
- [监控指标接口](#监控指标接口)

---

## 通用响应格式

所有 API 接口的响应均遵循统一的格式：

```json
{
  "api_version": 1,
  "status": "success | error",
  "message": "错误消息（仅在 status 为 error 时出现）",
  "data": { }
}
```

### 字段说明

| 字段 | 类型 | 描述 |
|------|------|------|
| `api_version` | `integer` | API 版本号，当前为 1 |
| `status` | `string` | 响应状态，"success" 或 "error" |
| `message` | `string` | 错误信息（仅在失败时返回） |
| `data` | `object` | 响应数据（具体内容根据接口不同而不同） |

---

## 告警接口

### 1. 创建/更新告警规则

**接口**: `POST /alarm/rules`

**请求体**:

```json
{
  "alert_name": "string",
  "alert_type": "string",
  "created_at": "string",
  "description": "string",
  "enabled": true,
  "expression": {
    "conditions": [
      {
        "operator": "string",
        "threshold": 0.0
      }
    ],
    "metric": "string",
    "stable": "string",
    "tags": [
      {
        "key1": "value1",
        "key2": "value2"
      }
    ]
  },
  "for": "string",
  "id": "string",
  "severity": "string",
  "summary": "string",
  "updated_at": "string"
}
```

**请求字段说明**:

| 字段 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `alert_name` | `string` | 是 | 告警规则名称，作为规则标识 |
| `alert_type` | `string` | 否 | 告警类型标签 |
| `created_at` | `string` | 否 | 创建时间（格式：YYYY-MM-DD HH:MM:SS） |
| `description` | `string` | 否 | 告警描述，支持模板变量（如 `{{host_ip}}`） |
| `enabled` | `boolean` | 否 | 是否启用规则，默认为 true |
| `expression` | `object` | 是 | 告警表达式对象 |
| `expression.conditions` | `array` | 是 | 条件数组 |
| `expression.conditions[].operator` | `string` | 是 | 比较运算符：`>`, `<`, `>=`, `<=`, `==`, `!=` |
| `expression.conditions[].threshold` | `number` | 是 | 阈值 |
| `expression.metric` | `string` | 是 | 监控指标名称（如 cpu, memory, disk 等） |
| `expression.stable` | `string` | 否 | 数据域（如 node, component 等） |
| `expression.tags` | `array` | 否 | 标签选择器数组，每个元素是键值对对象 |
| `for` | `string` | 否 | 持续时间（如 "1m", "5m", "1h"），默认为 "1s" |
| `id` | `string` | 否 | 规则 ID，不提供时使用 alert_name |
| `severity` | `string` | 否 | 严重程度："提示"、"一般"、"严重" |
| `summary` | `string` | 否 | 告警摘要 |
| `updated_at` | `string` | 否 | 更新时间（格式：YYYY-MM-DD HH:MM:SS） |

**可选字段**（在请求中可使用但不在 UserAlertRule 结构中）:

| 字段 | 类型 | 描述 |
|------|------|------|
| `window` | `string` | 时间窗口（如 "5m"），默认为 "5m" |
| `eval_every` | `string` | 评估间隔（如 "1s"），默认为 "1s" |
| `for_times` | `integer` | 触发次数阈值 |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "string",
    "message": "Rule created/updated successfully"
  }
}
```

---

### 2. 获取所有告警规则

**接口**: `GET /alarm/rules`

**请求参数**: 无

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": [
    {
      "alert_name": "string",
      "alert_type": "string",
      "created_at": "string",
      "description": "string",
      "enabled": true,
      "expression": {
        "conditions": [
          {
            "operator": "string",
            "threshold": 0.0
          }
        ],
        "metric": "string",
        "stable": "string",
        "tags": [
          {
            "key": "value"
          }
        ]
      },
      "for": "string",
      "id": "string",
      "severity": "string",
      "summary": "string",
      "updated_at": "string"
    }
  ]
}
```

**响应字段说明**: 与创建接口相同的 UserAlertRule 对象数组。

---

### 3. 获取单个告警规则

**接口**: `GET /alarm/rules/{id}`

**路径参数**:

| 参数 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `id` | `string` | 是 | 规则 ID |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "alert_name": "string",
    "alert_type": "string",
    "created_at": "string",
    "description": "string",
    "enabled": true,
    "expression": {
      "conditions": [
        {
          "operator": "string",
          "threshold": 0.0
        }
      ],
      "metric": "string",
      "stable": "string",
      "tags": []
    },
    "for": "string",
    "id": "string",
    "severity": "string",
    "summary": "string",
    "updated_at": "string"
  }
}
```

---

### 4. 更新告警规则

**接口**: `POST /alarm/rules/{id}/update`

**路径参数**:

| 参数 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `id` | `string` | 是 | 规则 ID |

**请求体**: 与创建接口相同

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "string",
    "message": "Rule updated successfully"
  }
}
```

---

### 5. 删除告警规则

**接口**: `POST /alarm/rules/{id}/delete`

**路径参数**:

| 参数 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `id` | `string` | 是 | 规则 ID |

**请求体**: 无

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "string"
  }
}
```

---

### 6. 获取告警事件列表

**接口**: `GET /alarm/events`

**查询参数**:

| 参数 | 类型 | 必填 | 默认值 | 描述 |
|------|------|------|--------|------|
| `duration` | `string` | 否 | "24h" | 查询时间范围（如 "1h", "24h"） |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": [
    {
      "annotations": {
        "description": "string",
        "summary": "string"
      },
      "created_at": "YYYY-MM-DD HH:MM:SS",
      "ends_at": "YYYY-MM-DD HH:MM:SS",
      "fingerprint": "string",
      "id": "string",
      "labels": {
        "alert_type": "string",
        "alertname": "string",
        "host_ip": "string",
        "metrics": "string",
        "severity": "string",
        "value": "string"
      },
      "starts_at": "YYYY-MM-DD HH:MM:SS",
      "status": "string",
      "updated_at": "YYYY-MM-DD HH:MM:SS"
    }
  ]
}
```

**事件字段说明**:

| 字段 | 类型 | 描述 |
|------|------|------|
| `annotations` | `object` | 注解信息 |
| `annotations.description` | `string` | 详细描述（支持模板渲染） |
| `annotations.summary` | `string` | 摘要信息 |
| `created_at` | `string` | 创建时间 |
| `ends_at` | `string` | 结束时间（未结束时为空） |
| `fingerprint` | `string` | 事件指纹，唯一标识 |
| `id` | `string` | 事件 ID（与 fingerprint 相同） |
| `labels` | `object` | 标签集合 |
| `labels.alert_type` | `string` | 告警类型 |
| `labels.alertname` | `string` | 告警规则名称 |
| `labels.host_ip` | `string` | 主机 IP |
| `labels.metrics` | `string` | 相关指标 |
| `labels.severity` | `string` | 严重程度："提示"、"一般"、"严重" |
| `labels.value` | `string` | 触发值 |
| `starts_at` | `string` | 开始时间 |
| `status` | `string` | 状态："inactive"、"pending"、"firing"、"resolved" |
| `updated_at` | `string` | 更新时间 |

---

### 7. 获取告警事件计数

**接口**: `GET /alarm/count`

**查询参数**:

| 参数 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `status` | `string` | 是 | 事件状态："pending"、"firing"、"resolved"、"inactive" |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "status": "string",
    "count": 0
  }
}
```

---

### 8. 手动创建组件告警事件

**接口**: `POST /alert/component`

**请求体**:

```json
{
  "host_ip": "string",
  "instance_id": "string",
  "uuid": "string",
  "index": "string",
  "rule_id": "string",
  "status": "string"
}
```

**请求字段说明**:

| 字段 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `host_ip` | `string` | 是 | 主机 IP |
| `instance_id` | `string` | 是 | 组件实例 ID |
| `uuid` | `string` | 是 | 组件 UUID |
| `index` | `string` | 是 | 组件索引 |
| `rule_id` | `string` | 否 | 规则 ID，默认为 "component" |
| `status` | `string` | 否 | 组件状态 |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "fingerprint": "string"
  }
}
```

---

## BMC 接口

### 1. 获取机箱 BMC 信息

**接口**: `GET /box/bmc`

**查询参数**:

| 参数 | 类型 | 必填 | 默认值 | 描述 |
|------|------|------|--------|------|
| `box_id` | `integer` | 是 | - | 机箱 ID |
| `duration` | `string` | 否 | "5m" | 查询时间范围（如 "1m", "5m"） |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "box_id": 0,
    "fan_0_speed": 0,
    "fan_1_speed": 0,
    "sensor": {
      "sensor_group_name": [
        {
          "timestamp": 0,
          "sensor_name": "string",
          "sensor_value": 0.0
        }
      ]
    }
  }
}
```

**响应字段说明**:

| 字段 | 类型 | 描述 |
|------|------|------|
| `box_id` | `integer` | 机箱 ID |
| `fan_0_speed` | `integer` | 风扇 0 转速 |
| `fan_1_speed` | `integer` | 风扇 1 转速 |
| `sensor` | `object` | 传感器数据，按组分类 |
| `sensor[group_name]` | `array` | 指定传感器组的时间序列数据 |
| `sensor[group_name][].timestamp` | `integer` | 时间戳 |
| `sensor[group_name][].sensor_name` | `string` | 传感器名称 |
| `sensor[group_name][].sensor_value` | `number` | 传感器值 |

---

### 2. 设置风扇转速

**接口**: `POST /box/bmc/fan_speed`

**请求体**:

```json
{
  "box_id": 0,
  "fan_speed": 0
}
```

**请求字段说明**:

| 字段 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `box_id` | `integer` | 是 | 机箱 ID |
| `fan_speed` | `integer` | 是 | 风扇转速（-128 到 127 之间） |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": [1, 2, 3]
}
```

**说明**: 
- `data` 字段包含 IPMI 原始响应字节数组
- 风扇转速转换：实际值 = 输入值 + 128（范围 0-255）

---

## 节点接口

### 1. 获取节点列表

**接口**: `GET /node`

**查询参数**:

| 参数 | 类型 | 必填 | 描述 |
|------|------|------|------|
| `box_id` | `integer` | 否 | 按机箱 ID 筛选 |
| `host_ip` | `string` | 否 | 按主机 IP 筛选（返回单个对象而非数组） |

**响应数据** (不带 `host_ip` 参数):

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes": [
      {
        "box_id": 0,
        "slot_id": 0,
        "cpu_id": 0,
        "host_ip": "string",
        "hostname": "string",
        "status": "string",
        "box_type": "string",
        "board_type": "string",
        "cpu_type": "string",
        "os_type": "string",
        "resource_type": "string",
        "cpu_arch": "string",
        "manufacturer": "string",
        "serial_number": "string",
        "production_date": "string",
        "gpu": [
          {
            "index": 0,
            "name": "string"
          }
        ],
        "updated_at": 0,
        "component": [
          {
            "instance_id": "string",
            "uuid": "string",
            "index": 0,
            "config": {
              "name": "string",
              "id": "string"
            },
            "state": "string",
            "resource": {
              "cpu": {
                "load": 0.0
              },
              "memory": {
                "mem_used": 0,
                "mem_limit": 0
              },
              "network": {
                "tx": 0,
                "rx": 0
              }
            }
          }
        ]
      }
    ]
  }
}
```

**响应数据** (带 `host_ip` 参数):

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "box_id": 0,
    "slot_id": 0,
    "cpu_id": 0,
    "host_ip": "string",
    "hostname": "string",
    "status": "string",
    "box_type": "string",
    "board_type": "string",
    "cpu_type": "string",
    "os_type": "string",
    "resource_type": "string",
    "cpu_arch": "string",
    "manufacturer": "string",
    "serial_number": "string",
    "production_date": "string",
    "gpu": [],
    "updated_at": 0,
    "component": []
  }
}
```

**字段说明**:

| 字段 | 类型 | 描述 |
|------|------|------|
| `box_id` | `integer` | 机箱 ID |
| `slot_id` | `integer` | 插槽 ID |
| `cpu_id` | `integer` | CPU ID |
| `host_ip` | `string` | 主机 IP 地址 |
| `hostname` | `string` | 主机名 |
| `status` | `string` | 节点状态 |
| `box_type` | `string` | 机箱类型 |
| `board_type` | `string` | 板卡类型 |
| `cpu_type` | `string` | CPU 类型 |
| `os_type` | `string` | 操作系统类型 |
| `resource_type` | `string` | 资源类型 |
| `cpu_arch` | `string` | CPU 架构 |
| `manufacturer` | `string` | 制造商 |
| `serial_number` | `string` | 序列号 |
| `production_date` | `string` | 生产日期 |
| `gpu` | `array` | GPU 设备列表 |
| `gpu[].index` | `integer` | GPU 索引 |
| `gpu[].name` | `string` | GPU 名称 |
| `updated_at` | `integer` | 更新时间戳（毫秒） |
| `component` | `array` | 业务组件列表 |
| `component[].instance_id` | `string` | 组件实例 ID |
| `component[].uuid` | `string` | 组件 UUID |
| `component[].index` | `integer` | 组件索引 |
| `component[].config` | `object` | 组件配置 |
| `component[].config.name` | `string` | 组件名称 |
| `component[].config.id` | `string` | 组件配置 ID |
| `component[].state` | `string` | 组件状态："RUNNING"、"PAUSED"、"STOPPED" |
| `component[].resource` | `object` | 组件资源使用情况 |
| `component[].resource.cpu.load` | `number` | CPU 负载 |
| `component[].resource.memory.mem_used` | `integer` | 已用内存（字节） |
| `component[].resource.memory.mem_limit` | `integer` | 内存限制（字节） |
| `component[].resource.network.tx` | `integer` | 发送字节数 |
| `component[].resource.network.rx` | `integer` | 接收字节数 |

---

## 监控指标接口

### 1. 获取节点指标快照

**接口**: `GET /node/metrics`

**请求参数**: 无

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes_metrics": [
      {
        "bmc_company": 0,
        "bmc_version": "string",
        "board_type": "string",
        "box_id": 0,
        "box_type": "string",
        "component": [],
        "cpu_arch": "string",
        "cpu_id": 0,
        "cpu_type": "string",
        "host_ip": "string",
        "hostname": "string",
        "id": 0,
        "ipmb_address": 0,
        "latest_container_metrics": {
          "container_count": 0,
          "paused_count": 0,
          "running_count": 0,
          "stopped_count": 0,
          "timestamp": 0
        },
        "latest_cpu_metrics": {
          "core_allocated": 0,
          "core_count": 0,
          "current": 0.0,
          "load_avg_15m": 0.0,
          "load_avg_1m": 0.0,
          "load_avg_5m": 0.0,
          "power": 0.0,
          "temperature": 0.0,
          "timestamp": 0,
          "usage_percent": 0.0,
          "voltage": 0.0
        },
        "latest_disk_metrics": {
          "disk_count": 0,
          "disks": [
            {
              "device": "string",
              "free": 0,
              "mount_point": "string",
              "timestamp": 0,
              "total": 0,
              "usage_percent": 0.0,
              "used": 0
            }
          ],
          "timestamp": 0
        },
        "latest_gpu_metrics": {
          "gpu_count": 0,
          "gpus": [
            {
              "compute_usage": 0.0,
              "current": 0.0,
              "index": 0,
              "mem_total": 0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "name": "string",
              "power": 0.0,
              "temperature": 0.0,
              "timestamp": 0,
              "voltage": 0.0
            }
          ],
          "timestamp": 0
        },
        "latest_memory_metrics": {
          "free": 0,
          "timestamp": 0,
          "total": 0,
          "usage_percent": 0.0,
          "used": 0
        },
        "latest_network_metrics": {
          "network_count": 0,
          "networks": [
            {
              "interface": "string",
              "rx_bytes": 0,
              "rx_errors": 0,
              "rx_packets": 0,
              "rx_rate": 0,
              "timestamp": 0,
              "tx_bytes": 0,
              "tx_errors": 0,
              "tx_packets": 0,
              "tx_rate": 0
            }
          ],
          "timestamp": 0
        },
        "latest_sensor_metrics": {
          "sensor_count": 0,
          "sensors": [],
          "timestamp": 0
        },
        "module_type": 0,
        "os_type": "string",
        "resource_type": "string",
        "service_port": 0,
        "slot_id": 0,
        "srio_id": 0,
        "status": "string",
        "updated_at": 0
      }
    ]
  }
}
```

**字段说明**:

#### 顶层字段

| 字段 | 类型 | 描述 |
|------|------|------|
| `bmc_company` | `integer` | BMC 厂商代码 |
| `bmc_version` | `string` | BMC 版本 |
| `board_type` | `string` | 板卡类型 |
| `box_id` | `integer` | 机箱 ID |
| `box_type` | `string` | 机箱类型 |
| `component` | `array` | 组件列表（格式同 /node 接口） |
| `cpu_arch` | `string` | CPU 架构 |
| `cpu_id` | `integer` | CPU ID |
| `cpu_type` | `string` | CPU 类型 |
| `host_ip` | `string` | 主机 IP |
| `hostname` | `string` | 主机名 |
| `id` | `integer` | 节点 ID |
| `ipmb_address` | `integer` | IPMB 地址 |
| `module_type` | `integer` | 模块类型 |
| `os_type` | `string` | 操作系统类型 |
| `resource_type` | `string` | 资源类型 |
| `service_port` | `integer` | 服务端口 |
| `slot_id` | `integer` | 插槽 ID |
| `srio_id` | `integer` | SRIO ID |
| `status` | `string` | 节点状态 |
| `updated_at` | `integer` | 更新时间戳（毫秒） |

#### latest_container_metrics（容器指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `container_count` | `integer` | 容器总数 |
| `running_count` | `integer` | 运行中容器数 |
| `paused_count` | `integer` | 暂停的容器数 |
| `stopped_count` | `integer` | 停止的容器数 |
| `timestamp` | `integer` | 时间戳（秒） |

#### latest_cpu_metrics（CPU 指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `core_allocated` | `integer` | 已分配核心数 |
| `core_count` | `integer` | 核心总数 |
| `current` | `number` | 电流（安培） |
| `load_avg_1m` | `number` | 1 分钟平均负载 |
| `load_avg_5m` | `number` | 5 分钟平均负载 |
| `load_avg_15m` | `number` | 15 分钟平均负载 |
| `power` | `number` | 功率（瓦特） |
| `temperature` | `number` | 温度（摄氏度） |
| `usage_percent` | `number` | 使用率（百分比） |
| `voltage` | `number` | 电压（伏特） |
| `timestamp` | `integer` | 时间戳（秒） |

#### latest_memory_metrics（内存指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `total` | `integer` | 总内存（字节） |
| `used` | `integer` | 已用内存（字节） |
| `free` | `integer` | 空闲内存（字节） |
| `usage_percent` | `number` | 使用率（百分比） |
| `timestamp` | `integer` | 时间戳（秒） |

#### latest_disk_metrics（磁盘指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `disk_count` | `integer` | 磁盘分区数 |
| `disks` | `array` | 磁盘分区列表 |
| `disks[].device` | `string` | 设备名 |
| `disks[].mount_point` | `string` | 挂载点 |
| `disks[].total` | `integer` | 总容量（字节） |
| `disks[].used` | `integer` | 已用容量（字节） |
| `disks[].free` | `integer` | 空闲容量（字节） |
| `disks[].usage_percent` | `number` | 使用率（百分比） |
| `disks[].timestamp` | `integer` | 时间戳（秒） |
| `timestamp` | `integer` | 时间戳（秒） |

#### latest_gpu_metrics（GPU 指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `gpu_count` | `integer` | GPU 数量 |
| `gpus` | `array` | GPU 列表 |
| `gpus[].index` | `integer` | GPU 索引 |
| `gpus[].name` | `string` | GPU 名称 |
| `gpus[].compute_usage` | `number` | 计算使用率（百分比） |
| `gpus[].mem_usage` | `number` | 显存使用率（百分比） |
| `gpus[].mem_used` | `integer` | 已用显存（字节） |
| `gpus[].mem_total` | `integer` | 总显存（字节） |
| `gpus[].temperature` | `number` | 温度（摄氏度） |
| `gpus[].power` | `number` | 功率（瓦特） |
| `gpus[].current` | `number` | 电流（安培） |
| `gpus[].voltage` | `number` | 电压（伏特） |
| `gpus[].timestamp` | `integer` | 时间戳（秒） |
| `timestamp` | `integer` | 时间戳（秒） |

#### latest_network_metrics（网络指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `network_count` | `integer` | 网络接口数 |
| `networks` | `array` | 网络接口列表 |
| `networks[].interface` | `string` | 网卡名称 |
| `networks[].rx_bytes` | `integer` | 接收字节数 |
| `networks[].tx_bytes` | `integer` | 发送字节数 |
| `networks[].rx_packets` | `integer` | 接收包数 |
| `networks[].tx_packets` | `integer` | 发送包数 |
| `networks[].rx_errors` | `integer` | 接收错误数 |
| `networks[].tx_errors` | `integer` | 发送错误数 |
| `networks[].rx_rate` | `integer` | 接收速率（字节/秒） |
| `networks[].tx_rate` | `integer` | 发送速率（字节/秒） |
| `networks[].timestamp` | `integer` | 时间戳（秒） |
| `timestamp` | `integer` | 时间戳（秒） |

#### latest_sensor_metrics（传感器指标）

| 字段 | 类型 | 描述 |
|------|------|------|
| `sensor_count` | `integer` | 传感器数量 |
| `sensors` | `array` | 传感器列表（结构待定） |
| `timestamp` | `integer` | 时间戳（秒） |

---

### 2. 获取节点历史指标

**接口**: `GET /node/historical-metrics`

**查询参数**:

| 参数 | 类型 | 必填 | 默认值 | 描述 |
|------|------|------|--------|------|
| `host_ip` | `string` | 是 | - | 主机 IP |
| `time_range` | `string` | 否 | "1m" | 时间范围（如 "1m", "5m", "1h"） |
| `metrics` | `string` | 否 | 全部 | 指标类型，逗号分隔（如 "cpu,memory,disk"） |

**响应数据**:

```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "historical_metrics": {
      "box_id": 0,
      "cpu_id": 0,
      "host_ip": "string",
      "slot_id": 0,
      "time_range": "string",
      "metrics": {
        "cpu": [
          {
            "timestamp": 0,
            "usage_percent": 0.0,
            "load_avg_1m": 0.0,
            "load_avg_5m": 0.0,
            "load_avg_15m": 0.0,
            "core_count": 0,
            "core_allocated": 0,
            "temperature": 0.0,
            "voltage": 0.0,
            "current": 0.0,
            "power": 0.0
          }
        ],
        "memory": [
          {
            "timestamp": 0,
            "total": 0,
            "used": 0,
            "free": 0,
            "usage_percent": 0.0
          }
        ],
        "disk": {
          "device_name": [
            {
              "timestamp": 0,
              "device": "string",
              "mount_point": "string",
              "total": 0,
              "used": 0,
              "free": 0,
              "usage_percent": 0.0
            }
          ]
        },
        "gpu": {
          "gpu_index": [
            {
              "timestamp": 0,
              "index": 0,
              "name": "string",
              "compute_usage": 0.0,
              "mem_usage": 0.0,
              "mem_used": 0,
              "mem_total": 0,
              "temperature": 0.0,
              "power": 0.0
            }
          ]
        },
        "network": {
          "interface_name": [
            {
              "interface": "string",
              "rx_bytes": 0,
              "tx_bytes": 0,
              "rx_packets": 0,
              "tx_packets": 0,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 0,
              "tx_rate": 0,
              "timestamp": 0
            }
          ]
        },
        "sensor": {}
      }
    }
  }
}
```

**字段说明**:

#### 顶层字段

| 字段 | 类型 | 描述 |
|------|------|------|
| `box_id` | `integer` | 机箱 ID |
| `cpu_id` | `integer` | CPU ID |
| `host_ip` | `string` | 主机 IP |
| `slot_id` | `integer` | 插槽 ID |
| `time_range` | `string` | 查询的时间范围 |
| `metrics` | `object` | 指标数据集合 |

#### metrics.cpu（CPU 时间序列）

| 字段 | 类型 | 描述 |
|------|------|------|
| `timestamp` | `integer` | 时间戳（秒） |
| `usage_percent` | `number` | 使用率（百分比） |
| `load_avg_1m` | `number` | 1 分钟平均负载 |
| `load_avg_5m` | `number` | 5 分钟平均负载 |
| `load_avg_15m` | `number` | 15 分钟平均负载 |
| `core_count` | `integer` | 核心总数 |
| `core_allocated` | `integer` | 已分配核心数 |
| `temperature` | `number` | 温度（摄氏度） |
| `voltage` | `number` | 电压（伏特） |
| `current` | `number` | 电流（安培） |
| `power` | `number` | 功率（瓦特） |

#### metrics.memory（内存时间序列）

| 字段 | 类型 | 描述 |
|------|------|------|
| `timestamp` | `integer` | 时间戳（秒） |
| `total` | `integer` | 总内存（字节） |
| `used` | `integer` | 已用内存（字节） |
| `free` | `integer` | 空闲内存（字节） |
| `usage_percent` | `number` | 使用率（百分比） |

#### metrics.disk（磁盘时间序列）

- **结构**: `{ "device_name": [array of points] }`
- 每个设备作为一个键，值为该设备的时间序列数组

| 字段 | 类型 | 描述 |
|------|------|------|
| `timestamp` | `integer` | 时间戳（秒） |
| `device` | `string` | 设备名 |
| `mount_point` | `string` | 挂载点 |
| `total` | `integer` | 总容量（字节） |
| `used` | `integer` | 已用容量（字节） |
| `free` | `integer` | 空闲容量（字节） |
| `usage_percent` | `number` | 使用率（百分比） |

#### metrics.gpu（GPU 时间序列）

- **结构**: `{ "gpu_index": [array of points] }`
- 每个 GPU 索引作为一个键，值为该 GPU 的时间序列数组

| 字段 | 类型 | 描述 |
|------|------|------|
| `timestamp` | `integer` | 时间戳（秒） |
| `index` | `integer` | GPU 索引 |
| `name` | `string` | GPU 名称 |
| `compute_usage` | `number` | 计算使用率（百分比） |
| `mem_usage` | `number` | 显存使用率（百分比） |
| `mem_used` | `integer` | 已用显存（字节） |
| `mem_total` | `integer` | 总显存（字节） |
| `temperature` | `number` | 温度（摄氏度） |
| `power` | `number` | 功率（瓦特） |

#### metrics.network（网络时间序列）

- **结构**: `{ "interface_name": [array of points] }`
- 每个网卡名称作为一个键，值为该网卡的时间序列数组

| 字段 | 类型 | 描述 |
|------|------|------|
| `interface` | `string` | 网卡名称 |
| `rx_bytes` | `integer` | 接收字节数 |
| `tx_bytes` | `integer` | 发送字节数 |
| `rx_packets` | `integer` | 接收包数 |
| `tx_packets` | `integer` | 发送包数 |
| `rx_errors` | `integer` | 接收错误数 |
| `tx_errors` | `integer` | 发送错误数 |
| `rx_rate` | `integer` | 接收速率（字节/秒） |
| `tx_rate` | `integer` | 发送速率（字节/秒） |
| `timestamp` | `integer` | 时间戳（秒） |

#### metrics.sensor（传感器数据）

- **结构**: 根据 BMC 模块返回的传感器数据而定
- 通常按传感器组分类，类似 BMC 接口的响应格式

---

## 数据类型说明

### 时间格式

- **日期时间字符串**: `YYYY-MM-DD HH:MM:SS` 格式（如 "2025-01-15 14:30:00"）
- **时间戳**: 整数，单位为秒或毫秒（根据字段而定）
- **时长字符串**: `<数字><单位>` 格式（如 "1m", "5m", "1h", "24h"）
  - 单位: `s`（秒）, `m`（分钟）, `h`（小时）

### 数值单位

- **内存/存储**: 字节（bytes）
- **网络速率**: 字节/秒（bytes/sec）
- **温度**: 摄氏度（°C）
- **电压**: 伏特（V）
- **电流**: 安培（A）
- **功率**: 瓦特（W）
- **百分比**: 0-100 的浮点数

### 枚举值

#### 告警严重程度
- `提示` (Info)
- `一般` (Warn)
- `严重` (Critical)

#### 告警状态
- `inactive` - 未激活
- `pending` - 待触发
- `firing` - 触发中
- `resolved` - 已解决

#### 组件状态
- `RUNNING` - 运行中
- `PAUSED` - 暂停
- `STOPPED` - 停止

#### 比较运算符
- `>` - 大于
- `<` - 小于
- `>=` - 大于等于
- `<=` - 小于等于
- `==` - 等于
- `!=` - 不等于

---

## 错误响应

所有接口在发生错误时返回统一格式：

```json
{
  "api_version": 1,
  "status": "error",
  "message": "错误描述信息",
  "data": {}
}
```

### 常见 HTTP 状态码

| 状态码 | 说明 |
|--------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 404 | 资源未找到 |
| 500 | 服务器内部错误 |

---

## 附录

### 示例：创建告警规则

```bash
curl -X POST http://localhost:8080/alarm/rules \
  -H "Content-Type: application/json" \
  -d '{
    "alert_name": "high_cpu_usage",
    "alert_type": "metric",
    "description": "节点 {{host_ip}} CPU 使用率过高，当前值：{{value}}",
    "enabled": true,
    "expression": {
      "stable": "node",
      "metric": "cpu",
      "conditions": [
        {
          "operator": ">",
          "threshold": 80
        }
      ],
      "tags": [
        {
          "resource_type": "compute"
        }
      ]
    },
    "for": "5m",
    "severity": "一般",
    "summary": "CPU 使用率告警"
  }'
```

### 示例：查询历史指标

```bash
curl -X GET "http://localhost:8080/node/historical-metrics?host_ip=192.168.1.100&time_range=1h&metrics=cpu,memory"
```

---

## 版本信息

- **文档版本**: 1.0
- **API 版本**: 1
- **生成日期**: 2025-10-09

