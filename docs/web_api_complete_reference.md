# Web 层 API 接口文档

## 📋 **接口概览**

本文档描述了 YW2 系统的所有 Web 层 API 接口，包括请求体、响应体和参数说明。

### **基础响应格式**
所有接口都遵循统一的响应格式：
```json
{
  "api_version": 1,
  "status": "success|error",
  "message": "响应消息（可选）",
  "data": "响应数据"
}
```

---

## 🔗 **节点管理接口**

### **GET /node**
获取所有节点信息，支持按 `box_id` 和 `host_ip` 过滤。

#### **请求参数**
- `box_id` (可选): 机箱ID，整数类型
- `host_ip` (可选): 主机IP地址，字符串类型

#### **请求示例**
```bash
GET /node
GET /node?box_id=1
GET /node?host_ip=192.168.1.100
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes": [
      {
        "box_id": 1,
        "slot_id": 1,
        "cpu_id": 1,
        "srio_id": 0,
        "host_ip": "192.168.1.100",
        "hostname": "node-001",
        "service_port": 23980,
        "box_type": "计算I型",
        "board_type": "GPU",
        "cpu_type": "Phytium,D2000/8",
        "os_type": "Kylin Linux Advanced Server V10",
        "resource_type": "GPU I",
        "cpu_arch": "aarch64",
        "gpu": [
          {
            "index": 0,
            "name": "NVIDIA GeForce RTX 3080"
          }
        ]
      }
    ]
  }
}
```

#### **单节点查询响应**
当使用 `host_ip` 参数时，返回单个节点对象：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "box_id": 1,
    "slot_id": 1,
    "cpu_id": 1,
    "srio_id": 0,
    "host_ip": "192.168.1.100",
    "hostname": "node-001",
    "service_port": 23980,
    "box_type": "计算I型",
    "board_type": "GPU",
    "cpu_type": "Phytium,D2000/8",
    "os_type": "Kylin Linux Advanced Server V10",
    "resource_type": "GPU I",
    "cpu_arch": "aarch64",
    "gpu": [
      {
        "index": 0,
        "name": "NVIDIA GeForce RTX 3080"
      }
    ]
  }
}
```

---

## 📊 **指标监控接口**

### **GET /node/metrics**
获取所有节点的实时指标数据。

#### **请求示例**
```bash
GET /node/metrics
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes_metrics": [
      {
        "box_id": 1,
        "slot_id": 1,
        "cpu_id": 1,
        "host_ip": "192.168.1.100",
        "hostname": "node-001",
        "status": "online",
        "box_type": "计算I型",
        "board_type": "GPU",
        "cpu_type": "Phytium,D2000/8",
        "os_type": "Kylin Linux Advanced Server V10",
        "resource_type": "GPU I",
        "cpu_arch": "aarch64",
        "service_port": 23980,
        "srio_id": 0,
        "updated_at": 1703123456,
        "latest_cpu_metrics": {
          "usage_percent": 23.5,
          "load_avg_1m": 0.75,
          "load_avg_5m": 0.61,
          "load_avg_15m": 0.52,
          "core_count": 16,
          "core_allocated": 8,
          "temperature": 58.2,
          "voltage": 1.1,
          "current": 2.3,
          "power": 25.6,
          "timestamp": 1703123456
        },
        "latest_memory_metrics": {
          "total": 34359738368,
          "used": 17179869184,
          "free": 17179869184,
          "usage_percent": 50.0,
          "timestamp": 1703123456
        },
        "latest_network_metrics": {
          "network_count": 1,
          "networks": [
            {
              "interface": "eth0",
              "rx_bytes": 123456,
              "tx_bytes": 654321,
              "rx_packets": 1000,
              "tx_packets": 900,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 2048,
              "tx_rate": 4096,
              "timestamp": 1703123456
            }
          ],
          "timestamp": 1703123456
        },
        "latest_disk_metrics": {
          "disk_count": 1,
          "disks": [
            {
              "device": "/dev/sda1",
              "mount_point": "/",
              "total": 536870912000,
              "used": 268435456000,
              "free": 268435456000,
              "usage_percent": 50.0,
              "timestamp": 1703123456
            }
          ],
          "timestamp": 1703123456
        },
        "latest_gpu_metrics": {
          "gpu_count": 2,
          "gpus": [
            {
              "index": 0,
              "name": "NVIDIA RTX 3080",
              "compute_usage": 45.0,
              "mem_usage": 30.0,
              "mem_used": 3221225472,
              "mem_total": 10737418240,
              "temperature": 65.0,
              "power": 120.5,
              "current": 0.0,
              "voltage": 0.0,
              "timestamp": 1703123456
            }
          ],
          "timestamp": 1703123456
        },
        "latest_container_metrics": {
          "container_count": 3,
          "running_count": 2,
          "paused_count": 0,
          "stopped_count": 1,
          "timestamp": 1703123456
        },
        "latest_sensor_metrics": {
          "sensor_count": 0,
          "sensors": [],
          "timestamp": 1703123456
        },
        "component": [
          {
            "instance_id": "biz-001",
            "uuid": "comp-abc",
            "index": 0,
            "config": {
              "name": "container-0",
              "id": "abcd1234"
            },
            "state": "RUNNING",
            "resource": {
              "cpu": {"load": 10.5},
              "memory": {"mem_used": 134217728, "mem_limit": 268435456},
              "network": {"tx": 10240, "rx": 20480}
            }
          }
        ]
      }
    ]
  }
}
```

### **GET /node/historical-metrics**
获取指定节点的历史指标数据。

#### **请求参数**
- `host_ip` (必需): 主机IP地址
- `time_range` (可选): 时间范围，默认 "1m"
- `metrics` (可选): 指标类型，逗号分隔，如 "cpu,memory,gpu"

#### **请求示例**
```bash
GET /node/historical-metrics?host_ip=192.168.1.100&time_range=5m&metrics=cpu,memory,gpu
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "historical_metrics": {
      "box_id": 1,
      "slot_id": 1,
      "cpu_id": 1,
      "host_ip": "192.168.1.100",
      "time_range": "5m",
      "metrics": {
        "cpu": [
          {
            "timestamp": "2024-01-01T12:00:00Z",
            "usage_percent": 23.5,
            "load_avg_1m": 0.75,
            "load_avg_5m": 0.61,
            "load_avg_15m": 0.52,
            "core_count": 16,
            "core_allocated": 8,
            "temperature": 58.2,
            "voltage": 1.1,
            "current": 2.3,
            "power": 25.6
          },
          {
            "timestamp": "2024-01-01T12:01:00Z",
            "usage_percent": 25.2,
            "load_avg_1m": 0.82,
            "load_avg_5m": 0.65,
            "load_avg_15m": 0.55,
            "core_count": 16,
            "core_allocated": 8,
            "temperature": 59.1,
            "voltage": 1.1,
            "current": 2.4,
            "power": 26.1
          }
        ],
        "memory": [
          {
            "timestamp": "2024-01-01T12:00:00Z",
            "total": 34359738368,
            "used": 17179869184,
            "free": 17179869184,
            "usage_percent": 50.0
          },
          {
            "timestamp": "2024-01-01T12:01:00Z",
            "total": 34359738368,
            "used": 17895696384,
            "free": 16464041984,
            "usage_percent": 52.1
          }
        ],
        "disk": {
          "/dev/sda1": [
            {
              "timestamp": "2024-01-01T12:00:00Z",
              "device": "/dev/sda1",
              "mount_point": "/",
              "total": 536870912000,
              "used": 268435456000,
              "free": 268435456000,
              "usage_percent": 50.0
            },
            {
              "timestamp": "2024-01-01T12:01:00Z",
              "device": "/dev/sda1",
              "mount_point": "/",
              "total": 536870912000,
              "used": 275146342400,
              "free": 261724569600,
              "usage_percent": 51.3
            }
          ]
        },
        "gpu": {
          "0": [
            {
              "timestamp": "2024-01-01T12:00:00Z",
              "index": 0,
              "name": "NVIDIA RTX 3080",
              "compute_usage": 45.0,
              "mem_usage": 30.0,
              "mem_used": 3221225472,
              "mem_total": 10737418240,
              "temperature": 65.0,
              "power": 120.5
            },
            {
              "timestamp": "2024-01-01T12:01:00Z",
              "index": 0,
              "name": "NVIDIA RTX 3080",
              "compute_usage": 48.3,
              "mem_usage": 32.1,
              "mem_used": 3448061952,
              "mem_total": 10737418240,
              "temperature": 66.2,
              "power": 125.8
            }
          ]
        },
        "network": {
          "eth0": [
            {
              "timestamp": "2024-01-01T12:00:00Z",
              "interface": "eth0",
              "rx_bytes": 123456,
              "tx_bytes": 654321,
              "rx_packets": 1000,
              "tx_packets": 900,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 2048,
              "tx_rate": 4096
            },
            {
              "timestamp": "2024-01-01T12:01:00Z",
              "interface": "eth0",
              "rx_bytes": 245678,
              "tx_bytes": 789012,
              "rx_packets": 2100,
              "tx_packets": 1900,
              "rx_errors": 0,
              "tx_errors": 0,
              "rx_rate": 2156,
              "tx_rate": 4200
            }
          ]
        },
        "sensor": {
          "fan_0_speed": [
            {"timestamp": "2024-01-01T12:00:00Z", "value": 1200},
            {"timestamp": "2024-01-01T12:01:00Z", "value": 1250}
          ],
          "temperature": [
            {"timestamp": "2024-01-01T12:00:00Z", "value": 45.2},
            {"timestamp": "2024-01-01T12:01:00Z", "value": 46.1}
          ],
          "voltage": [
            {"timestamp": "2024-01-01T12:00:00Z", "value": 12.1},
            {"timestamp": "2024-01-01T12:01:00Z", "value": 12.0}
          ]
        }
      }
    }
  }
}
```

---

## 🚨 **告警管理接口**

### **POST /alarm/rules**
创建或更新告警规则。

#### **请求体**
```json
{
  "alert_name": "CPU使用率过高",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {
        "operator": ">=",
        "threshold": 80.0
      }
    ],
    "tags": [
      {
        "host_ip": "192.168.1.100"
      }
    ]
  },
  "for_duration": "30s",
  "severity": "Warn",
  "summary": "CPU使用率超过80%",
  "description": "主机 {host_ip} 的CPU使用率为 {value}%",
  "alert_type": "resource",
  "enabled": true
}
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "cpu_high",
    "message": "Rule created/updated successfully"
  }
}
```

### **GET /alarm/rules**
获取所有告警规则。

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": [
    {
      "id": "cpu_high",
      "alert_name": "CPU使用率过高",
      "expression": {
        "stable": "cpu",
        "metric": "usage_percent",
        "conditions": [
          {
            "operator": ">=",
            "threshold": 80.0
          }
        ],
        "tags": [
          {
            "host_ip": "192.168.1.100"
          }
        ]
      },
      "for_duration": "30s",
      "severity": "Warn",
      "summary": "CPU使用率超过80%",
      "description": "主机 {host_ip} 的CPU使用率为 {value}%",
      "alert_type": "resource",
      "enabled": true,
      "created_at": "2024-01-01T12:00:00Z",
      "updated_at": "2024-01-01T12:00:00Z"
    }
  ]
}
```

### **GET /alarm/rules/{id}**
获取指定ID的告警规则。

#### **请求示例**
```bash
GET /alarm/rules/cpu_high
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "cpu_high",
    "alert_name": "CPU使用率过高",
    "expression": {
      "stable": "cpu",
      "metric": "usage_percent",
      "conditions": [
        {
          "operator": ">=",
          "threshold": 80.0
        }
      ],
      "tags": [
        {
          "host_ip": "192.168.1.100"
        }
      ]
    },
    "for_duration": "30s",
    "severity": "Warn",
    "summary": "CPU使用率超过80%",
    "description": "主机 {host_ip} 的CPU使用率为 {value}%",
    "alert_type": "resource",
    "enabled": true,
    "created_at": "2024-01-01T12:00:00Z",
    "updated_at": "2024-01-01T12:00:00Z"
  }
}
```

### **POST /alarm/rules/{id}/update**
更新指定ID的告警规则。

#### **请求体**
```json
{
  "alert_name": "CPU使用率过高（更新）",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {
        "operator": ">=",
        "threshold": 85.0
      }
    ],
    "tags": [
      {
        "host_ip": "192.168.1.100"
      }
    ]
  },
  "for_duration": "1m",
  "severity": "Critical",
  "summary": "CPU使用率超过85%",
  "description": "主机 {host_ip} 的CPU使用率为 {value}%",
  "alert_type": "resource",
  "enabled": true
}
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "cpu_high",
    "message": "Rule updated successfully"
  }
}
```

### **POST /alarm/rules/{id}/delete**
删除指定ID的告警规则。

#### **请求示例**
```bash
POST /alarm/rules/cpu_high/delete
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "cpu_high"
  }
}
```

### **GET /alarm/events**
获取告警事件列表。

#### **请求参数**
- `duration` (可选): 查询时间范围，默认 "24h"

#### **请求示例**
```bash
GET /alarm/events?duration=7d
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": [
    {
      "fingerprint": "cpu_high|host_ip=192.168.1.100",
      "labels": {
        "host_ip": "192.168.1.100"
      },
      "status": "firing",
      "summary": "CPU使用率超过80%",
      "description": "主机 192.168.1.100 的CPU使用率为 85.5%",
      "starts_at": "2024-01-01T12:00:00Z",
      "ends_at": "",
      "created_at": "2024-01-01T12:00:00Z",
      "updated_at": "2024-01-01T12:05:00Z"
    }
  ]
}
```

### **GET /alarm/count**
获取指定状态的告警事件数量。

#### **请求参数**
- `status` (必需): 告警状态，可选值：pending, firing, resolved, inactive

#### **请求示例**
```bash
GET /alarm/count?status=firing
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "status": "firing",
    "count": 5
  }
}
```

### **POST /alert/component**
上报业务组件状态异常。

#### **请求体**
```json
{
  "host_ip": "192.168.1.100",
  "instance_id": "biz-001",
  "uuid": "comp-abc",
  "index": 0,
  "status": "FAILED"
}
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "fingerprint": "192.168.1.100_biz-001_comp-abc_0"
  }
}
```

---

## 🔧 **BMC 管理接口**

### **GET /box/bmc**
获取指定机箱的BMC信息。

#### **请求参数**
- `box_id` (必需): 机箱ID
- `duration` (可选): 查询时间范围，默认 "5m"

#### **请求示例**
```bash
GET /box/bmc?box_id=1&duration=10m
```

#### **响应示例**
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "box_id": 1,
    "fan_0_speed": 1200,
    "fan_1_speed": 1150,
    "sensor": {
      "fan_0_speed": [
        {"time": "2024-01-01T12:00:00Z", "value": 1200},
        {"time": "2024-01-01T12:01:00Z", "value": 1250}
      ],
      "temperature": [
        {"time": "2024-01-01T12:00:00Z", "value": 45.2},
        {"time": "2024-01-01T12:01:00Z", "value": 46.1}
      ],
      "voltage": [
        {"time": "2024-01-01T12:00:00Z", "value": 12.1},
        {"time": "2024-01-01T12:01:00Z", "value": 12.0}
      ]
    }
  }
}
```

---

## 📡 **数据上报接口**

### **POST /heartbeat**
节点上报心跳状态信息。

#### **请求体**
```json
{
  "api_version": 1,
  "data": {
    "box_id": 1,
    "slot_id": 1,
    "cpu_id": 1,
    "srio_id": 0,
    "host_ip": "192.168.10.29",
    "hostname": "localhost.localdomain",
    "service_port": 23980,
    "box_type": "计算I型",
    "board_type": "GPU",
    "cpu_type": "Phytium,D2000/8",
    "os_type": "Kylin Linux Advanced Server V10",
    "resource_type": "GPU I",
    "cpu_arch": "aarch64",
    "gpu": [
      {
        "index": 0,
        "name": "NVIDIA GeForce RTX 3080"
      },
      {
        "index": 1,
        "name": "NVIDIA GeForce RTX 3080"
      }
    ]
  }
}
```

#### **响应示例**
```json
{
  "code": 0,
  "message": "success"
}
```

### **POST /resource**
节点上报资源使用情况。

#### **请求体**
```json
{
  "api_version": 1,
  "data": {
    "host_ip": "192.168.10.29",
    "resource": {
      "cpu": {
        "usage_percent": 23.5,
        "load_avg_1m": 0.75,
        "load_avg_5m": 0.61,
        "load_avg_15m": 0.52,
        "core_count": 16,
        "core_allocated": 8,
        "temperature": 58.2,
        "voltage": 1.1,
        "current": 2.3,
        "power": 25.6
      },
      "memory": {
        "total": 34359738368,
        "used": 17179869184,
        "free": 17179869184,
        "usage_percent": 50.0
      },
      "network": [
        {
          "interface": "eth0",
          "rx_bytes": 123456,
          "tx_bytes": 654321,
          "rx_packets": 1000,
          "tx_packets": 900,
          "rx_errors": 0,
          "tx_errors": 0,
          "rx_rate": 2048,
          "tx_rate": 4096
        }
      ],
      "disk": [
        {
          "device": "/dev/sda1",
          "mount_point": "/",
          "total": 536870912000,
          "used": 268435456000,
          "free": 268435456000,
          "usage_percent": 50.0
        }
      ],
      "gpu": [
        {
          "index": 0,
          "name": "NVIDIA RTX 3080",
          "compute_usage": 45.0,
          "mem_usage": 30.0,
          "mem_used": 3221225472,
          "mem_total": 10737418240,
          "temperature": 65.0,
          "power": 120.5
        }
      ],
      "gpu_allocated": 1,
      "gpu_num": 2
    },
    "component": [
      {
        "instance_id": "biz-001",
        "uuid": "comp-abc",
        "index": 0,
        "config": {
          "name": "container-0",
          "id": "abcd1234"
        },
        "state": "RUNNING",
        "resource": {
          "cpu": {
            "load": 10.5
          },
          "memory": {
            "mem_used": 134217728,
            "mem_limit": 268435456
          },
          "network": {
            "tx": 10240,
            "rx": 20480
          }
        }
      }
    ]
  }
}
```

#### **响应示例**
```json
{
  "code": 0,
  "message": "success"
}
```

---

## ❌ **错误响应格式**

### **400 Bad Request**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "Invalid request parameters",
  "data": {}
}
```

### **404 Not Found**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "Resource not found",
  "data": {}
}
```

### **500 Internal Server Error**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "Internal server error",
  "data": {}
}
```

---

## 📝 **注意事项**

1. **时间格式**: 所有时间字段使用 ISO 8601 格式 (YYYY-MM-DDTHH:MM:SSZ)
2. **IP地址格式**: 使用点分十进制格式 (xxx.xxx.xxx.xxx)
3. **数值精度**: 浮点数保留适当精度，整数使用 int64 格式
4. **错误处理**: 所有接口都包含完整的错误处理机制
5. **版本控制**: 所有接口都包含 api_version 字段，当前版本为 1
6. **内容类型**: 所有响应都使用 application/json 内容类型
