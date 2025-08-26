
# API 文档

## 1. /box/bmc

获取指定 box 的最新 BMC 风扇数据和传感器数据。

### 请求方法
`GET /box/bmc`

### 请求参数

| 参数名 | 类型 | 必需 | 默认值 | 说明 |
|--------|------|------|--------|------|
| box_id | integer | 是 | - | 要查询的 box ID |
| duration | string | 否 | "5m" | 传感器数据的时间范围，支持格式如 "5m", "1h", "30s" 等 |

### 请求示例

```bash
# 获取 box_id=1 的 BMC 数据，使用默认时间范围
GET /box/bmc?box_id=1

# 获取 box_id=1 的 BMC 数据，指定时间范围为 10 分钟
GET /box/bmc?box_id=1&duration=10m
```

### 响应格式

#### 成功响应 (200)

```json
{
  "api_version": 1,
  "data": {
    "box_id": 1,
    "fan": [
      {
        "fanseq": 0,
        "fanmode": 16,
        "fanspeed": 50,
        "alarm_type": 1,
        "work_mode": 0,
        "speed_unit": "duty",
        "duty_cycle": 50
      },
      {
        "fanseq": 1,
        "fanmode": 16,
        "fanspeed": 60,
        "alarm_type": 1,
        "work_mode": 0,
        "speed_unit": "duty",
        "duty_cycle": 60
      }
    ],
    "sensor": {
      "temperature": [
        {
          "host_ip": "192.168.67.181",
          "sensor_name": "cpu_temp",
          "value": 45.5,
          "unit": "℃",
          "timestamp": "2023-12-01T10:30:00Z"
        }
      ],
      "voltage": [
        {
          "host_ip": "192.168.67.181",
          "sensor_name": "cpu_voltage",
          "value": 1.2,
          "unit": "V",
          "timestamp": "2023-12-01T10:30:00Z"
        }
      ]
    }
  },
  "status": "success"
}
```

#### 错误响应

**缺少 box_id 参数 (400)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "box_id parameter is required"
}
```

**无效的 box_id 参数 (400)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "Invalid box_id parameter"
}
```

**Box 不存在或无数据 (404)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "Box not found or no BMC data available"
}
```

### 响应字段说明

#### 风扇数据 (fan)
| 字段名 | 类型 | 说明 |
|--------|------|------|
| fanseq | integer | 风扇序号 (0, 1) |
| fanmode | integer | 原始模式字节 |
| fanspeed | integer | 原始速度字节 |
| alarm_type | integer | 报警类型 (高4位) |
| work_mode | integer | 工作模式 (0=自动, 1=手动) |
| speed_unit | string | 速度单位 ("level" 或 "duty") |
| speed_level | integer | 速度等级 (1=低速, 2=中速, 3=高速)，仅当 speed_unit="level" 时存在 |
| duty_cycle | integer | 占空比百分比 (1-100)，仅当 speed_unit="duty" 时存在 |

#### 传感器数据 (sensor)
传感器数据按类型分组，每个类型包含传感器数组：
- `temperature`: 温度传感器数据
- `voltage`: 电压传感器数据
- `current`: 电流传感器数据
- `power`: 功率传感器数据

每个传感器记录包含：
| 字段名 | 类型 | 说明 |
|--------|------|------|
| host_ip | string | 主机 IP 地址 |
| sensor_name | string | 传感器名称 |
| value | number | 传感器数值 |
| unit | string | 单位 |
| timestamp | string | 时间戳 (ISO 8601 格式) |
