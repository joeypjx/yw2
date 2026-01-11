# 节点管理接口文档

本文档说明节点管理相关的HTTP API接口。

## 接口概览

节点管理接口提供节点信息查询和数据导出功能，包括：
- 节点列表查询
- 节点详情查询
- 节点历史数据导出

## 接口列表

### 1. 查询节点列表或单个节点

**接口路径：** `GET /node`

**功能描述：** 查询节点信息，支持查询所有节点、按IP查询单个节点、按机箱号查询节点列表。

**查询参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| host_ip | string | 否 | 节点IP地址，指定时返回单个节点对象 |
| box_id | integer | 否 | 机箱编号（1-9），指定时返回该机箱下的所有节点列表 |

**请求示例：**

```bash
# 查询所有节点
GET /node

# 查询指定IP的节点
GET /node?host_ip=192.168.10.58

# 查询指定机箱的所有节点
GET /node?box_id=1
```

**响应格式：**

成功响应（查询所有节点或按机箱查询）：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes": [
      {
        "host_ip": "192.168.10.58",
        "hostname": "localhost.localdomain",
        "box_id": 0,
        "slot_id": 0,
        "board_type": "GPU",
        "box_type": "计算I型",
        "cpu_id": 1,
        "cpu_type": " D2000 ",
        "cpu_arch": "aarch64",
        "manufacturer": "715",
        "os_type": "UOS Server 20 Military",
        "production_date": "2024-05-11",
        "resource_type": "GPU I",
        "serial_number": "030MPV10E5000831",
        "status": "online",
        "component": [
          {
            "instance_id": "instance_001",
            "uuid": "550e8400-e29b-41d4-a716-446655440000",
            "index": 0,
            "config": {
              "name": "my-container",
              "id": "container_123"
            },
            "state": "running",
            "type": "docker",
            "resource": {
              "cpu": {
                "load": 0.5
              },
              "memory": {
                "mem_used": 1073741824,
                "mem_limit": 2147483648,
                "mem_usage": 50.0
              },
              "network": {
                "tx": 1024000,
                "rx": 2048000,
                "rx_rate": 1024.0,
                "tx_rate": 512.0
              }
            }
          }
        ],
        "gpu": [
          {
            "index": 0,
            "name": " Iluvatar MR-V50A"
          }
        ],
        "updated_at": 1761211760113
      }
    ]
  }
}
```

成功响应（查询单个节点，节点存在）：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "host_ip": "192.168.10.58",
    "hostname": "localhost.localdomain",
    "box_id": 0,
    "slot_id": 0,
    "board_type": "GPU",
    "box_type": "计算I型",
    "cpu_id": 1,
    "cpu_type": " D2000 ",
    "cpu_arch": "aarch64",
    "manufacturer": "715",
    "os_type": "UOS Server 20 Military",
    "production_date": "2024-05-11",
    "resource_type": "GPU I",
    "serial_number": "030MPV10E5000831",
    "status": "online",
    "component": [
      {
        "instance_id": "instance_001",
        "uuid": "550e8400-e29b-41d4-a716-446655440000",
        "index": 0,
        "config": {
          "name": "my-container",
          "id": "container_123"
        },
        "state": "running",
        "type": "docker",
        "resource": {
          "cpu": {
            "load": 0.5
          },
          "memory": {
            "mem_used": 1073741824,
            "mem_limit": 2147483648,
            "mem_usage": 50.0
          },
          "network": {
            "tx": 1024000,
            "rx": 2048000,
            "rx_rate": 1024.0,
            "tx_rate": 512.0
          }
        }
      }
    ],
    "gpu": [
      {
        "index": 0,
        "name": " Iluvatar MR-V50A"
      }
    ],
    "updated_at": 1761211760113
  }
}
```

成功响应（查询单个节点，节点不存在）：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| host_ip | string | 节点IP地址 |
| hostname | string | 节点主机名 |
| box_id | integer | 机箱编号 |
| slot_id | integer | 槽位号 |
| board_type | string | 板卡类型 |
| box_type | string | 机箱类型 |
| cpu_id | integer | CPU ID |
| cpu_type | string | CPU类型 |
| cpu_arch | string | CPU架构 |
| manufacturer | string | 制造商 |
| os_type | string | 操作系统类型 |
| production_date | string | 生产日期 |
| resource_type | string | 资源类型 |
| serial_number | string | 序列号 |
| status | string | 节点状态（online/offline） |
| component | array | 组件列表（容器/进程），每个元素是一个组件对象 |
| component[].instance_id | string | 组件实例ID |
| component[].uuid | string | 组件UUID |
| component[].index | integer | 组件索引 |
| component[].config | object | 组件配置信息 |
| component[].config.name | string | 组件名称 |
| component[].config.id | string | 组件ID（容器ID） |
| component[].state | string | 组件状态 |
| component[].type | string | 组件类型（如"docker"） |
| component[].resource | object | 组件资源使用情况 |
| component[].resource.cpu | object | CPU资源 |
| component[].resource.cpu.load | float | CPU负载 |
| component[].resource.memory | object | 内存资源 |
| component[].resource.memory.mem_used | integer | 已使用内存（字节） |
| component[].resource.memory.mem_limit | integer | 内存限制（字节） |
| component[].resource.memory.mem_usage | float | 内存使用率百分比 |
| component[].resource.network | object | 网络资源 |
| component[].resource.network.tx | integer | 网络发送字节数 |
| component[].resource.network.rx | integer | 网络接收字节数 |
| component[].resource.network.rx_rate | float | 网络接收速率（字节/秒） |
| component[].resource.network.tx_rate | float | 网络发送速率（字节/秒） |
| gpu | array | GPU列表，包含GPU索引和名称 |
| gpu[].index | integer | GPU索引 |
| gpu[].name | string | GPU名称 |
| updated_at | integer | 最后更新时间（毫秒时间戳） |

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 2. 导出节点历史数据

**接口路径：** `GET /node/export`

**功能描述：** 导出指定节点在指定时间范围内的历史监控数据，支持按指标类型过滤。

**查询参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| ip | string | 是 | 节点IP地址 |
| start_time | integer | 是 | 开始时间（秒级时间戳） |
| end_time | integer | 是 | 结束时间（秒级时间戳） |
| type | string | 否 | 指标类型，多个用逗号分隔（如"cpu,memory"），可选值：cpu、memory、network、disk、gpu，特殊值"system"表示所有类型 |

**请求示例：**

```bash
# 导出指定时间范围内的所有指标数据
GET /node/export?ip=192.168.10.58&start_time=1234567890&end_time=1234567990

# 导出指定类型的指标数据
GET /node/export?ip=192.168.10.58&start_time=1234567890&end_time=1234567990&type=cpu,memory
```

**响应格式：**

成功响应：
```json
{
  "code": 200,
  "data": [
    {
      "start_time": 1234567890,
      "end_time": 1234567990,
      "ip": "192.168.10.58",
      "type": "cpu,memory",
      "data": [
        {
          "timestamp": 1234567890,
          "cpu_usage_percent": 45.5,
          "memory_usage_percent": 60.2,
          "disk_/data_usage_percent": 30.5,
          "network_eth0_rx_rate": 1000000,
          "network_eth0_tx_rate": 500000,
          "gpu_0_compute_usage": 80.0,
          "gpu_0_mem_usage": 50.0
        }
      ]
    }
  ]
}
```

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| start_time | integer | 开始时间（秒级时间戳） |
| end_time | integer | 结束时间（秒级时间戳） |
| ip | string | 节点IP地址 |
| type | string | 指标类型 |
| data | array | 数据点列表 |
| timestamp | integer | 数据点时间戳（秒级） |
| cpu_usage_percent | float | CPU使用率百分比 |
| memory_usage_percent | float | 内存使用率百分比 |
| disk_{mount_point}_usage_percent | float | 磁盘使用率百分比（按挂载点） |
| network_{interface}_rx_rate | integer | 网络接收速率（字节/秒） |
| network_{interface}_tx_rate | integer | 网络发送速率（字节/秒） |
| gpu_{index}_compute_usage | float | GPU计算使用率百分比 |
| gpu_{index}_mem_usage | float | GPU内存使用率百分比 |

**错误响应：**

```json
{
  "code": 400,
  "data": "missing required parameters: ip, start_time, end_time"
}
```

## 注意事项

1. 节点状态（status）根据节点最后更新时间判断，默认10秒内更新为在线（online），超过10秒为离线（offline）
2. 节点信息主要存储在内存缓存中，不直接写入数据库
3. 导出数据的时间范围必须满足：start_time < end_time
4. 指标类型参数支持的特殊值：
   - 空值或未指定：导出所有类型（cpu、memory、network、disk、gpu）
   - "system"：导出所有类型
   - 其他：按指定类型过滤
