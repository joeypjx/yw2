
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
    "fan_0_speed": 50,
    "fan_1_speed": 60,
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

#### 数据字段 (data)
| 字段名 | 类型 | 说明 |
|--------|------|------|
| box_id | integer | Box ID |
| fan_0_speed | integer | 风扇 0 的速度值 |
| fan_1_speed | integer | 风扇 1 的速度值 |
| sensor | object | 传感器数据对象，包含按类型分组的传感器数组 |

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

---

## 2. /box/reset_board

复位指定 box 的板卡。

### 请求方法
`POST /box/reset_board`

### 请求参数

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 要操作的 box ID |
| slot_id | array of integer | 是 | 要复位的槽位号列表，范围 1-12 |

### 请求示例

```bash
POST /box/reset_board
Content-Type: application/json

{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

### 响应格式

#### 成功响应 (200)

```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

#### 错误响应

**无效的 JSON 请求体 (400)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "invalid json body",
  "data": {}
}
```

**缺少必需参数 (400)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "box_id and slot_id are required integers",
  "data": {}
}
```

**槽位号超出范围 (400)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "slot_id is out of range",
  "data": {}
}
```

**操作失败 (500)**
```json
{
  "api_version": 1,
  "status": "error",
  "message": "操作失败的具体错误信息",
  "data": {}
}
```

---

## 3. /box/poweron_board

对指定 box 的板卡进行上电操作。

### 请求方法
`POST /box/poweron_board`

### 请求参数

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 要操作的 box ID |
| slot_id | array of integer | 是 | 要上电的槽位号列表，范围 1-12 |

### 请求示例

```bash
POST /box/poweron_board
Content-Type: application/json

{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

### 响应格式

#### 成功响应 (200)

当所有槽位操作成功或部分成功时返回：

```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

#### 错误响应

错误响应格式与 `/box/reset_board` 相同。

---

## 4. /box/poweroff_board

对指定 box 的板卡进行下电操作。

### 请求方法
`POST /box/poweroff_board`

### 请求参数

| 参数名 | 类型 | 必需 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 要操作的 box ID |
| slot_id | array of integer | 是 | 要下电的槽位号列表，范围 1-12 |

### 请求示例

```bash
POST /box/poweroff_board
Content-Type: application/json

{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

### 响应格式

#### 成功响应 (200)

当所有槽位操作成功或部分成功时返回：

```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

#### 错误响应

错误响应格式与 `/box/reset_board` 相同。

---

## 通用说明

### 板卡控制 API 通用规则

1. **槽位号范围**：所有 `slot_id` 必须在 1-12 之间
2. **支持多槽位**：可以同时操作多个槽位，传入槽位号数组
3. **部分成功**：`poweron_board` 和 `poweroff_board` 支持部分成功（部分槽位操作成功），此时仍返回成功状态
4. **Box IP 计算**：系统会根据 `box_id` 自动计算目标 IP 地址，格式为 `192.168.{box_id * 2}.180`
5. **超时设置**：操作默认超时时间为 10 秒
