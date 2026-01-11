# 告警管理接口文档

本文档说明告警规则和告警事件相关的HTTP API接口。

## 接口概览

告警管理接口提供告警规则管理和告警事件查询功能，包括：
- 告警规则管理（增删改查）
- 告警事件查询
- 告警统计

## 接口列表

### 1. 创建告警规则

**接口路径：** `POST /alarm/rules`

**功能描述：** 创建新的告警规则。

**请求体：**

```json
{
  "id": "rule_001",
  "alert_name": "CPU使用率过高",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {
        "operator": ">",
        "threshold": 80.0
      }
    ],
    "tags": []
  },
  "for": "5m",
  "severity": "严重",
  "summary": "CPU使用率超过80%",
  "description": "节点CPU使用率持续超过80%达到5分钟",
  "alert_type": "系统告警",
  "enabled": true
}
```

**请求参数说明：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | string | 是 | 告警规则ID（唯一标识符） |
| alert_name | string | 是 | 告警规则名称（唯一） |
| expression | object | 是 | 告警表达式 |
| expression.stable | string | 是 | 数据源类型（如"cpu"、"disk"、"memory"、"alive"） |
| expression.metric | string | 是 | 指标名称（如"usage_percent"、"alive"） |
| expression.conditions | array | 是 | 条件数组 |
| expression.conditions[].operator | string | 是 | 操作符（如">"、"=="、"<"） |
| expression.conditions[].threshold | number | 是 | 阈值 |
| expression.tags | array | 是 | 标签数组，用于过滤特定资源（如[{"mount_point":"/data"}]） |
| for | string | 是 | 持续时间（如"5s"、"30s"、"1h"） |
| severity | string | 是 | 告警等级（用户自定义） |
| summary | string | 是 | 告警摘要 |
| description | string | 是 | 告警详情（支持占位符{{}}） |
| alert_type | string | 是 | 告警类型（用户自定义） |
| enabled | boolean | 否 | 是否启用，默认true |

**请求示例：**

```bash
POST /alarm/rules
Content-Type: application/json

{
  "id": "rule_001",
  "alert_name": "CPU使用率过高",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [{"operator": ">", "threshold": 80.0}],
    "tags": []
  },
  "for": "5m",
  "severity": "严重",
  "summary": "CPU使用率超过80%",
  "description": "节点CPU使用率持续超过80%达到5分钟",
  "alert_type": "系统告警"
}
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "规则标识",
    "message": "Rule created/updated successfully"
  }
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

### 2. 获取所有告警规则

**接口路径：** `GET /alarm/rules`

**功能描述：** 获取所有告警规则列表。

**请求示例：**

```bash
GET /alarm/rules
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": [
    {
      "id": "cpu_high",
      "alert_name": "cpu_high",
      "expression": {
        "stable": "cpu",
        "metric": "usage_percent",
        "conditions": [{"operator": ">", "threshold": 80.0}],
        "tags": [
          {
            "host_type": "server"
          }
        ]
      },
      "for": "2m",
      "severity": "一般",
      "summary": "CPU使用率过高",
      "description": "当CPU使用率持续超过80%时触发告警，可能表示系统负载过高或存在性能问题",
      "alert_type": "resource",
      "enabled": true,
      "created_at": "2025-10-23 07:09:13.340371+00",
      "updated_at": "2025-10-23 07:09:13.340371+00"
    }
  ]
}
```

### 3. 获取特定告警规则

**接口路径：** `GET /alarm/rules/{id}`

**功能描述：** 根据ID获取特定告警规则。

**路径参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | string | 是 | 告警规则ID |

**请求示例：**

```bash
GET /alarm/rules/rule_001
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "cpu_high",
    "alert_name": "cpu_high",
    "expression": {
      "stable": "cpu",
      "metric": "usage_percent",
      "conditions": [{"operator": ">", "threshold": 80.0}],
      "tags": [
        {
          "host_type": "server"
        }
      ]
    },
    "for": "2m",
    "severity": "一般",
    "summary": "CPU使用率过高",
    "description": "当CPU使用率持续超过80%时触发告警，可能表示系统负载过高或存在性能问题",
    "alert_type": "resource",
    "enabled": true,
    "created_at": "2025-10-23 07:09:13.340371+00",
    "updated_at": "2025-10-23 07:09:13.340371+00"
  }
}
```

**错误响应（规则不存在）：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 4. 更新告警规则

**接口路径：** `POST /alarm/rules/{id}/update`

**功能描述：** 更新指定ID的告警规则。

**路径参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | string | 是 | 告警规则ID |

**请求体：** 同创建告警规则，但ID必须与路径参数一致。

**请求示例：**

```bash
POST /alarm/rules/rule_001/update
Content-Type: application/json

{
  "id": "rule_001",
  "alert_name": "CPU使用率过高（已更新）",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [{"operator": ">", "threshold": 90.0}],
    "tags": []
  },
  "for": "10m",
  "severity": "严重",
  "summary": "CPU使用率超过90%",
  "description": "节点CPU使用率持续超过90%达到10分钟",
  "alert_type": "系统告警"
}
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "memory_high",
    "message": "Rule updated successfully"
  }
}
```

### 5. 删除告警规则

**接口路径：** `POST /alarm/rules/{id}/delete`

**功能描述：** 删除指定ID的告警规则。

**路径参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | string | 是 | 告警规则ID |

**请求示例：**

```bash
POST /alarm/rules/rule_001/delete
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "cpu_high"
  }
}
```

### 6. 获取告警事件列表

**接口路径：** `GET /alarm/events`

**功能描述：** 查询告警事件列表，支持多种过滤条件。

**查询参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| status | string | 否 | 告警状态（pending/firing/resolved） |
| severity | string | 否 | 告警严重程度 |
| alert_type | string | 否 | 告警类型 |
| host_ip | string | 否 | 节点IP地址 |
| box_id | integer | 否 | 机箱编号 |
| slot_id | integer | 否 | 槽位号 |
| description | string | 否 | 告警描述（模糊匹配） |
| start_time | string | 否 | 开始时间 |
| end_time | string | 否 | 结束时间 |
| stack_name | string | 否 | 堆栈名称 |
| component_name | string | 否 | 组件名称 |
| limit | integer | 否 | 返回数量限制，默认100，范围1-1000 |

**请求示例：**

```bash
# 查询所有告警事件（排除pending状态）
GET /alarm/events

# 查询firing状态的告警
GET /alarm/events?status=firing

# 查询指定节点的告警
GET /alarm/events?host_ip=192.168.10.58

# 查询指定时间范围内的告警
GET /alarm/events?start_time=2024-01-01T00:00:00Z&end_time=2024-01-01T23:59:59Z

# 组合查询
GET /alarm/events?status=firing&severity=严重&limit=50
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": [
    {
      "id": "规则标识|device=/dev/sda5|mount_point=/data|host_ip=192.168.10.58/32",
      "fingerprint": "规则标识|device=/dev/sda5|mount_point=/data|host_ip=192.168.10.58/32",
      "status": "firing",
      "starts_at": "2025-10-23 15:59:01",
      "ends_at": "",
      "created_at": "2025-10-23 15:59:01",
      "updated_at": "2025-10-23 15:59:01",
      "labels": {
        "alert_type": "硬件状态",
        "alertname": "规则标识",
        "host_ip": "192.168.10.58",
        "metrics": "total",
        "severity": "严重",
        "value": "426474012672.000000"
      },
      "annotations": {
        "description": "",
        "summary": "告警摘要"
      }
    }
  ]
}
```

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| id | string | 告警事件ID（由规则ID和标签组合生成） |
| fingerprint | string | 告警指纹（与ID相同，用于唯一标识告警） |
| status | string | 告警状态（pending/firing/resolved） |
| starts_at | string | 告警开始时间（格式：YYYY-MM-DD HH:MM:SS） |
| ends_at | string | 告警结束时间（格式：YYYY-MM-DD HH:MM:SS），空字符串表示告警仍在持续 |
| created_at | string | 创建时间（格式：YYYY-MM-DD HH:MM:SS） |
| updated_at | string | 更新时间（格式：YYYY-MM-DD HH:MM:SS） |
| labels | object | 告警标签，包含告警类型、名称、节点IP、指标、严重程度、值等信息 |
| labels.alert_type | string | 告警类型 |
| labels.alertname | string | 告警规则名称 |
| labels.host_ip | string | 节点IP地址 |
| labels.metrics | string | 指标名称 |
| labels.severity | string | 严重程度 |
| labels.value | string | 指标值（字符串格式） |
| annotations | object | 告警注释，包含描述和摘要 |
| annotations.description | string | 告警描述 |
| annotations.summary | string | 告警摘要 |

### 7. 获取特定告警事件

**接口路径：** `GET /alarm/events/{id}`

**功能描述：** 根据ID获取特定告警事件的详细信息。

**路径参数：**

| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | string | 是 | 告警事件ID |

**请求示例：**

```bash
GET /alarm/events/alert_001
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "id": "规则标识|device=/dev/sda5|mount_point=/data|host_ip=192.168.10.58/32",
    "fingerprint": "规则标识|device=/dev/sda5|mount_point=/data|host_ip=192.168.10.58/32",
    "status": "firing",
    "starts_at": "2025-10-23 15:59:01",
    "ends_at": "",
    "created_at": "2025-10-23 15:59:01",
    "updated_at": "2025-10-23 15:59:01",
    "labels": {
      "alert_type": "硬件状态",
      "alertname": "规则标识",
      "host_ip": "192.168.10.58",
      "metrics": "total",
      "severity": "严重",
      "value": "426474012672.000000"
    },
    "annotations": {
      "description": "",
      "summary": "告警摘要"
    }
  }
}
```

**响应字段说明：** 同告警事件列表接口

**错误响应（告警不存在）：**

```json
{
  "api_version": 1,
  "status": "error",
  "data": {}
}
```

### 8. 获取告警总数

**接口路径：** `GET /alarm/count`

**功能描述：** 获取告警事件总数。

**查询参数：** 无

**请求示例：**

```bash
GET /alarm/count
```

**响应格式：**

成功响应：
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "count": 100
  }
}
```

**响应字段说明：**

| 字段名 | 类型 | 说明 |
|--------|------|------|
| count | integer | 告警事件总数 |

## 注意事项

1. 告警规则ID必须唯一，告警规则名称（alert_name）也必须唯一
2. 告警表达式中的stable支持的值：cpu、disk、memory、network、gpu、alive
3. 告警表达式中的operator支持：>、<、==、>=、<=、!=
4. for字段支持的时间单位：s（秒）、m（分钟）、h（小时）
5. tags字段是数组类型，用于过滤特定资源，例如：`[{"mount_point":"/data"}]` 或 `[{"host_type":"server"}]`
6. 告警事件状态：
   - pending：待评估状态
   - firing：触发状态（告警条件满足）
   - resolved：已解决状态（告警条件不再满足）
7. 告警事件ID由规则ID和标签组合生成，格式为：`规则ID|标签1=值1|标签2=值2|...`
8. 告警事件的fingerprint与ID相同，用于唯一标识告警
9. 查询告警事件时，如果不提供任何过滤条件，默认返回除pending状态外的所有告警
10. limit参数用于限制返回的告警数量，防止返回过多数据，默认100，最大1000
11. 告警规则更新后，告警引擎会在下次评估周期重新评估该规则
12. 删除告警规则时，响应中只返回规则ID，不包含message字段
