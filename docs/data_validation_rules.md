# API 数据校验规则文档

本文档详细描述了系统中所有 API 接口的数据校验规则，包括字段验证、格式检查、范围限制等。

## 目录

- [通用校验规则](#通用校验规则)
- [节点心跳接口 (POST /heartbeat)](#节点心跳接口-post-heartbeat)
- [资源监控接口 (POST /resource)](#资源监控接口-post-resource)
- [错误响应格式](#错误响应格式)

---

## 通用校验规则

### 1. 请求体校验

所有 POST 接口都需要进行以下基础校验：

- **请求体不能为空**：如果请求体为空，返回 400 错误
- **JSON 格式校验**：请求体必须是有效的 JSON 格式，否则返回 400 错误
- **data 字段必需**：所有接口的请求体必须包含 `data` 字段，且不能为 `null`

### 2. IP 地址格式校验

所有 `host_ip` 字段必须符合以下规则：

- **格式要求**：必须是有效的 IPv4 地址格式
- **正则表达式**：`^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$`
- **段范围**：每个段必须在 0-255 范围内
- **示例**：
  - ✅ 有效：`192.168.1.1`、`10.0.0.1`、`172.16.0.1`
  - ❌ 无效：`256.1.1.1`、`192.168.1`、`192.168.1.1.1`

---

## 节点心跳接口 (POST /heartbeat)

### 接口信息

- **路径**：`POST /heartbeat`
- **Content-Type**：`application/json`
- **API 版本**：1

### 请求格式

```json
{
  "api_version": 1,
  "data": {
    "host_ip": "192.168.1.1",
    "box_id": 0,
    "slot_id": 0,
    "cpu_id": 1,
    "srio_id": 1,
    "hostname": "localhost.localdomain",
    "service_port": 23980,
    "box_type": "计算I型",
    "board_type": "GPU",
    "cpu_type": "D2000",
    "os_type": "UOS Server 20 Military",
    "resource_type": "GPU I",
    "cpu_arch": "aarch64",
    "gpu": [
      {
        "index": 0,
        "name": "Iluvatar MR-V50A"
      }
    ],
    "manufacturer": "715",
    "serial_number": "030MPV10E5000831",
    "production_date": "2024-05-11"
  }
}
```

### 校验规则

#### 必需字段

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `data.host_ip` | `string` | 必需，非空，有效 IPv4 格式 | `host_ip is required` / `host_ip cannot be empty` / `invalid host_ip format: {ip}` |

#### 可选字段校验

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `data.service_port` | `integer` | 如果存在，必须在 0-65535 范围内 | `service_port must be between 0 and 65535, got: {port}` |
| `data.box_id` | `integer` | 如果存在，必须非负 | `box_id must be non-negative, got: {box_id}` |
| `data.slot_id` | `integer` | 如果存在，必须非负 | `slot_id must be non-negative, got: {slot_id}` |
| `data.gpu[].index` | `integer` | 如果存在，必须非负 | `gpu index must be non-negative, got: {index}` |

### 校验流程

1. 检查请求体是否为空
2. 解析 JSON，检查格式是否正确
3. 检查是否包含 `data` 字段
4. 验证 `data.host_ip` 字段（必需）
5. 验证可选字段（如果存在）
6. 转换为 `Node` 对象
7. 保存到缓存

---

## 资源监控接口 (POST /resource)

### 接口信息

- **路径**：`POST /resource`
- **Content-Type**：`application/json`
- **API 版本**：1

### 请求格式

```json
{
  "api_version": 1,
  "data": {
    "host_ip": "192.168.1.1",
    "resource": {
      "cpu": {
        "usage_percent": 75.4,
        "load_avg_1m": 11.68,
        "load_avg_5m": 9.46,
        "load_avg_15m": 6.58,
        "core_count": 8,
        "core_allocated": 12,
        "temperature": 0,
        "voltage": 0,
        "current": 0,
        "power": 0
      },
      "memory": {
        "total": 15645540352,
        "used": 7241269248,
        "free": 8404271104,
        "usage_percent": 46.28
      },
      "network": [
        {
          "interface": "enaphyt4i0",
          "rx_bytes": 9183683391,
          "tx_bytes": 1975762894,
          "rx_packets": 13738817,
          "tx_packets": 3245933,
          "rx_errors": 0,
          "tx_errors": 0,
          "rx_rate": 20340.33,
          "tx_rate": 12002.17
        }
      ],
      "disk": [
        {
          "device": "/dev/sda4",
          "mount_point": "/",
          "total": 75125227520,
          "used": 28160962560,
          "free": 46964264960,
          "usage_percent": 37.49
        }
      ],
      "gpu": [
        {
          "index": 0,
          "name": "Iluvatar MR-V50A",
          "compute_usage": 0,
          "mem_usage": 1,
          "mem_used": 119537664,
          "mem_total": 17179869184,
          "temperature": 44,
          "power": 20
        }
      ],
      "gpu_allocated": 0,
      "gpu_num": 1
    },
    "component": [
      {
        "instance_id": "4c398af4-44df-4c1a-8e30-04ea9412c0c1",
        "uuid": "1233211234567",
        "index": 1,
        "config": {
          "name": "192.168.10.58:5000/pts1:latest",
          "id": "acb5c64aa2f2644cc957fc3261d52f3dbe5749b7edb481c540ce6be51c3c987a"
        },
        "state": "RUNNING",
        "resource": {
          "cpu": {
            "load": 15.87
          },
          "memory": {
            "mem_used": 104005632,
            "mem_limit": 15645540352,
            "mem_usage": 0.664
          },
          "network": {
            "rx": 0,
            "tx": 0,
            "rx_rate": 0,
            "tx_rate": 0
          }
        }
      }
    ]
  }
}
```

### 校验规则

#### 必需字段

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `data.host_ip` | `string` | 必需，非空，有效 IPv4 格式 | `host_ip is required` / `host_ip cannot be empty` / `invalid host_ip format: {ip}` |

#### Resource 字段校验

##### CPU 资源 (`data.resource.cpu`)

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `usage_percent` | `double` | 如果存在，必须在 0-100 范围内 | `cpu.usage_percent must be between 0 and 100, got: {value}` |
| `core_count` | `integer` | 如果存在，必须非负 | `cpu.core_count must be non-negative, got: {value}` |
| `core_allocated` | `integer` | 如果存在，必须非负 | `cpu.core_allocated must be non-negative, got: {value}` |

##### Memory 资源 (`data.resource.memory`)

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `usage_percent` | `double` | 如果存在，必须在 0-100 范围内 | `memory.usage_percent must be between 0 and 100, got: {value}` |
| `used` / `total` | `uint64` | 如果同时存在，`used` 不能大于 `total` | `memory.used cannot be greater than memory.total` |

##### Disk 数组 (`data.resource.disk[]`)

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `usage_percent` | `double` | 如果存在，必须在 0-100 范围内 | `disk[{index}].usage_percent must be between 0 and 100, got: {value}` |
| `used` / `total` | `uint64` | 如果同时存在，`used` 不能大于 `total` | `disk[{index}].used cannot be greater than disk[{index}].total` |

##### GPU 数组 (`data.resource.gpu[]`)

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `index` | `integer` | 如果存在，必须非负 | `gpu[{index}].index must be non-negative, got: {value}` |
| `compute_usage` | `double` | 如果存在，必须在 0-100 范围内 | `gpu[{index}].compute_usage must be between 0 and 100, got: {value}` |
| `mem_usage` | `double` | 如果存在，必须在 0-100 范围内 | `gpu[{index}].mem_usage must be between 0 and 100, got: {value}` |
| `mem_used` / `mem_total` | `uint64` | 如果同时存在，`mem_used` 不能大于 `mem_total` | `gpu[{index}].mem_used cannot be greater than gpu[{index}].mem_total` |

##### GPU 统计字段

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `gpu_allocated` | `integer` | 如果存在，必须非负 | `gpu_allocated must be non-negative, got: {value}` |
| `gpu_num` | `integer` | 如果存在，必须非负 | `gpu_num must be non-negative, got: {value}` |

#### Component 数组校验 (`data.component[]`)

##### Component Memory 资源 (`data.component[{index}].resource.memory`)

| 字段 | 类型 | 校验规则 | 错误信息 |
|------|------|---------|---------|
| `mem_usage` | `double` | 如果存在，必须在 0-1 范围内（表示比例） | `component[{index}].resource.memory.mem_usage must be between 0 and 1, got: {value}` |
| `mem_used` / `mem_limit` | `uint64` | 如果同时存在，`mem_used` 不能大于 `mem_limit` | `component[{index}].resource.memory.mem_used cannot be greater than mem_limit` |

### 校验流程

1. 检查请求体是否为空
2. 解析 JSON，检查格式是否正确
3. 检查是否包含 `data` 字段
4. 验证 `data.host_ip` 字段（必需）
5. 验证 `data.resource` 字段（如果存在）
   - CPU 资源校验
   - Memory 资源校验
   - Disk 数组校验
   - GPU 数组校验
   - GPU 统计字段校验
6. 验证 `data.component` 数组（如果存在）
   - Component Memory 资源校验
7. 转换为 `Resource` 对象
8. 保存到缓存和数据库

---

## 错误响应格式

### 标准错误响应

所有校验失败时，接口返回统一的错误响应格式：

```json
{
  "api_version": 1,
  "status": "error",
  "message": "错误描述信息",
  "data": {}
}
```

### HTTP 状态码

| 错误类型 | HTTP 状态码 | 说明 |
|---------|------------|------|
| 请求体为空 | `400 Bad Request` | 请求体为空 |
| JSON 格式错误 | `400 Bad Request` | JSON 解析失败 |
| 缺少 data 字段 | `400 Bad Request` | 请求体中缺少 `data` 字段 |
| 数据校验失败 | `400 Bad Request` | 字段校验不通过 |
| 对象转换失败 | `400 Bad Request` | JSON 无法转换为目标对象 |
| 数据库保存失败 | `500 Internal Server Error` | 保存到数据库时出错 |
| 其他异常 | `500 Internal Server Error` | 服务器内部错误 |

### 错误消息示例

#### 1. 请求体为空

```json
{
  "api_version": 1,
  "status": "error",
  "message": "empty request body",
  "data": {}
}
```

#### 2. JSON 格式错误

```json
{
  "api_version": 1,
  "status": "error",
  "message": "invalid JSON format: parse error at line 1, column 5",
  "data": {}
}
```

#### 3. 缺少 data 字段

```json
{
  "api_version": 1,
  "status": "error",
  "message": "missing 'data' field",
  "data": {}
}
```

#### 4. 数据校验失败

```json
{
  "api_version": 1,
  "status": "error",
  "message": "validation failed: host_ip is required",
  "data": {}
}
```

```json
{
  "api_version": 1,
  "status": "error",
  "message": "validation failed: invalid host_ip format: 256.1.1.1",
  "data": {}
}
```

```json
{
  "api_version": 1,
  "status": "error",
  "message": "validation failed: cpu.usage_percent must be between 0 and 100, got: 150.5",
  "data": {}
}
```

#### 5. 对象转换失败

```json
{
  "api_version": 1,
  "status": "error",
  "message": "failed to parse node data: [json.exception.type_error.302] type must be string, but is number",
  "data": {}
}
```

#### 6. 数据库保存失败

```json
{
  "api_version": 1,
  "status": "error",
  "message": "failed to save resource: connection timeout",
  "data": {}
}
```

### 成功响应格式

校验通过并成功处理后，返回成功响应：

```json
{
  "api_version": 1,
  "status": "success",
  "message": "heartbeat received",
  "data": {}
}
```

或

```json
{
  "api_version": 1,
  "status": "success",
  "message": "resource received",
  "data": {}
}
```

---

## 校验规则总结表

### 节点心跳接口 (/heartbeat)

| 字段路径 | 必需 | 类型 | 校验规则 | 范围/格式 |
|---------|------|------|---------|----------|
| `data.host_ip` | ✅ | `string` | 非空，IPv4 格式 | `0.0.0.0` - `255.255.255.255` |
| `data.service_port` | ❌ | `integer` | 非负 | `0` - `65535` |
| `data.box_id` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.slot_id` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.gpu[].index` | ❌ | `integer` | 非负 | `≥ 0` |

### 资源监控接口 (/resource)

| 字段路径 | 必需 | 类型 | 校验规则 | 范围/格式 |
|---------|------|------|---------|----------|
| `data.host_ip` | ✅ | `string` | 非空，IPv4 格式 | `0.0.0.0` - `255.255.255.255` |
| `data.resource.cpu.usage_percent` | ❌ | `double` | 百分比 | `0.0` - `100.0` |
| `data.resource.cpu.core_count` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.resource.cpu.core_allocated` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.resource.memory.usage_percent` | ❌ | `double` | 百分比 | `0.0` - `100.0` |
| `data.resource.memory.used` | ❌ | `uint64` | ≤ `total` | `≤ total` |
| `data.resource.disk[].usage_percent` | ❌ | `double` | 百分比 | `0.0` - `100.0` |
| `data.resource.disk[].used` | ❌ | `uint64` | ≤ `total` | `≤ total` |
| `data.resource.gpu[].index` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.resource.gpu[].compute_usage` | ❌ | `double` | 百分比 | `0.0` - `100.0` |
| `data.resource.gpu[].mem_usage` | ❌ | `double` | 百分比 | `0.0` - `100.0` |
| `data.resource.gpu[].mem_used` | ❌ | `uint64` | ≤ `mem_total` | `≤ mem_total` |
| `data.resource.gpu_allocated` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.resource.gpu_num` | ❌ | `integer` | 非负 | `≥ 0` |
| `data.component[].resource.memory.mem_usage` | ❌ | `double` | 比例 | `0.0` - `1.0` |
| `data.component[].resource.memory.mem_used` | ❌ | `uint64` | ≤ `mem_limit` | `≤ mem_limit` |

---

## 注意事项

1. **字段可选性**：除了 `host_ip` 字段为必需外，其他字段均为可选。如果字段存在，则必须符合相应的校验规则。

2. **数组校验**：对于数组类型的字段（如 `gpu[]`、`disk[]`、`component[]`），会遍历数组中的每个元素进行校验。

3. **逻辑校验**：除了格式和范围校验外，还包含逻辑校验，如 `used` 不能大于 `total`、`mem_used` 不能大于 `mem_limit` 等。

4. **错误信息**：所有校验失败的错误信息都会明确指出哪个字段、哪个索引（如果是数组）以及具体的错误原因。

5. **日志记录**：所有校验失败的情况都会记录到日志中，便于排查问题。

---

## 更新历史

- **2025-01-XX**：初始版本，包含节点心跳和资源监控接口的校验规则

