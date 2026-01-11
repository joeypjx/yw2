# BMC管理接口文档

本文档说明BMC管理和板卡控制相关的HTTP API接口。

## 接口概览

BMC管理接口提供BMC信息查询和板卡控制功能，包括：
- BMC信息查询
- 板卡复位控制
- 板卡电源控制
- 风扇速度控制

## 接口列表

### 1. 获取指定机箱的BMC数据

**接口路径：** `GET /box/bmc`

**功能描述：** 查询指定机箱的BMC信息，包括风扇速度和传感器数据。

**查询参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 机箱编号（1-9） |
| duration | string | 否 | 时间范围，默认"5m"，用于查询传感器历史数据 |

**请求示例：**

```bash
# 查询机箱1的BMC数据
GET /box/bmc?box_id=1

# 查询机箱1最近10分钟的BMC数据
GET /box/bmc?box_id=1&duration=10m
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "box_id": 1,
    "fan_0_speed": 3000,
    "fan_1_speed": 3100,
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
```

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| box_id | integer | 机箱编号 |
| fan_0_speed | integer | 风扇0转速（RPM），0表示风扇不存在 |
| fan_1_speed | integer | 风扇1转速（RPM），0表示风扇不存在 |
| sensor | object | 传感器数据（按传感器名称索引） |
| sensor.{sensor_name} | array | 传感器历史数据列表 |
| sensor.{sensor_name}[].timestamp | integer | 时间戳（秒级） |
| sensor.{sensor_name}[].host_ip | string | 节点IP地址 |
| sensor.{sensor_name}[].sensorseq | integer | 传感器序号 |
| sensor.{sensor_name}[].sensortype | integer | 传感器类型 |
| sensor.{sensor_name}[].sensorname | string | 传感器名称 |
| sensor.{sensor_name}[].sensorvalue_L | integer | 传感器数值低字节（小数部分） |
| sensor.{sensor_name}[].sensorvalue_H | integer | 传感器数值高字节（整数部分） |
| sensor.{sensor_name}[].sensor_value | float | 传感器实际数值（sensorvalue_H + sensorvalue_L * 0.01） |
| sensor.{sensor_name}[].sensoralmtype | integer | 告警类型 |

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 2. 重置机箱板卡

**接口路径：** `POST /box/reset_board`

**功能描述：** 复位指定机箱的指定板卡。

**请求体：**

```json
{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

**请求参数说明：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 机箱编号（1-9） |
| slot_id | array | 是 | 槽位号数组，范围1-12 |

**请求示例：**

```bash
POST /box/reset_board
Content-Type: application/json

{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 3. 开启机箱板卡电源

**接口路径：** `POST /box/poweron_board`

**功能描述：** 开启指定机箱的指定板卡电源。

**请求体：**

```json
{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

**请求参数说明：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 机箱编号（1-9） |
| slot_id | array | 是 | 槽位号数组，范围1-12 |

**请求示例：**

```bash
POST /box/poweron_board
Content-Type: application/json

{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

**响应格式：**

成功响应（全部成功或部分成功）：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 4. 关闭机箱板卡电源

**接口路径：** `POST /box/poweroff_board`

**功能描述：** 关闭指定机箱的指定板卡电源。

**请求体：**

```json
{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

**请求参数说明：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 机箱编号（1-9） |
| slot_id | array | 是 | 槽位号数组，范围1-12 |

**请求示例：**

```bash
POST /box/poweroff_board
Content-Type: application/json

{
  "box_id": 1,
  "slot_id": [1, 2, 3]
}
```

**响应格式：**

成功响应（全部成功或部分成功）：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {}
}
```

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 5. 设置机箱风扇速度

**接口路径：** `POST /box/bmc/fan_speed`

**功能描述：** 设置指定机箱的风扇速度。

**请求体：**

```json
{
  "box_id": 1,
  "fan_speed": 50
}
```

**请求参数说明：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| box_id | integer | 是 | 机箱编号（1-9） |
| fan_speed | integer | 是 | 风扇速度值，范围0-127，对应实际速度值128-255 |

**请求示例：**

```bash
POST /box/bmc/fan_speed
Content-Type: application/json

{
  "box_id": 1,
  "fan_speed": 50
}
```

**响应格式：**

成功响应（IPMI命令执行成功）：
```json
{
  "api_version": 1,
  "status": "success",
  "data": [0, 1, 2]
}
```

成功响应（IPMI命令执行失败，但不暴露错误信息）：
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
| data | array/object | IPMI命令响应字节数组（整数数组，每个元素范围0-255）。如果命令执行失败，data为空对象 `{}` |

**错误响应：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

## 注意事项

1. 板卡控制操作（复位、上电、下电）支持批量操作，可以同时对多个槽位进行操作
2. 槽位号范围：1-12（排除5和8，这两个槽位通常不使用）
3. 板卡复位操作要求全部成功，不接受部分成功（`acceptPartialSuccess = false`）
4. 板卡电源操作（上电、下电）接受部分成功，只要有一个槽位操作成功即返回成功（`acceptPartialSuccess = true`）
5. 风扇速度值范围：0-127，实际映射到IPMI协议的速度值范围是128-255
   - 小于0的值 → 128（最低速度）
   - 0 → 128（最低速度）
   - 127 → 255（最高速度）
   - 大于127的值 → 255（最高速度）
6. 风扇速度控制接口（`POST /box/bmc/fan_speed`）：
   - IPMI命令执行成功时，返回响应字节数组（整数数组）
   - IPMI命令执行失败时，也返回成功状态，但 `data` 为空对象 `{}`（不暴露内部错误信息）
7. BMC数据查询时，如果第一个IP地址没有数据，会自动尝试第二个IP地址（备用IP格式：`192.168.{box_id*2}.181`）
8. 机箱IP地址计算规则：使用槽位7的IP地址作为机箱IP地址
9. 传感器数据格式：每个传感器数据点包含完整的BMCSensorRow字段（timestamp、host_ip、sensorseq、sensortype、sensorname、sensorvalue_L、sensorvalue_H、sensor_value、sensoralmtype）
10. 传感器数据按传感器名称（sensorname）索引，每个传感器对应一个时序数据数组
