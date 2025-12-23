# Alert Routes 文档

本文档记录了 AlertRoutes 中所有已注册的 HTTP API 路由。

## 告警规则管理 (Alert Rules)

### POST /alarm/rules
**功能**: 创建告警规则

**请求体格式**:
```json
{
  "alert_name": "<规则名称>",
  "expression": {
    "stable": "<资源类型>",
    "metric": "<指标名称>",
    "conditions": [...],
    "tags": [...]
  },
  "for": "<持续时间>",
  "severity": "<严重程度>",
  "summary": "<摘要>",
  "description": "<描述>",
  "alert_type": "<告警类型>"
}
```

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "<规则ID>",
    "message": "Rule created successfully"
  }
}
```

---

### GET /alarm/rules
**功能**: 获取所有告警规则

**参数**: 无

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": [
    {
      "id": "<规则ID>",
      "alert_name": "<规则名称>",
      "expression": {...},
      "for": "<持续时间>",
      "severity": "<严重程度>",
      "summary": "<摘要>",
      "description": "<描述>",
      "alert_type": "<告警类型>",
      "created_at": "<创建时间>",
      "updated_at": "<更新时间>",
      "enabled": true
    }
  ]
}
```

---

### GET /alarm/rules/{id}
**功能**: 获取特定告警规则

**路径参数**:
- `id`: 告警规则ID

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "<规则ID>",
    "alert_name": "<规则名称>",
    "expression": {...},
    ...
  }
}
```

**错误响应** (规则不存在):
```json
{
  "api_version": 2,
  "status": "error",
  "message": "rule not found",
  "data": {}
}
```

---

### POST /alarm/rules/{id}/update
**功能**: 更新告警规则

**路径参数**:
- `id`: 告警规则ID

**请求体格式**: 与创建规则相同，但ID会从路径参数中获取

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "<规则ID>",
    "message": "Rule updated successfully"
  }
}
```

---

### POST /alarm/rules/{id}/delete
**功能**: 删除告警规则

**路径参数**:
- `id`: 告警规则ID

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "<规则ID>",
    "message": "Rule deleted successfully"
  }
}
```

---

## 告警事件查询 (Alert Events)

### GET /alarm/events
**功能**: 获取告警事件列表

**查询参数** (可选):
- `status`: 状态过滤 (pending/firing/resolved)
- `severity`: 严重程度过滤
- `alert_type`: 告警类型过滤
- `host_ip`: 主机IP过滤
- `box_id`: 机箱号过滤 (整数)
- `slot_id`: 板卡号过滤 (整数)
- `start_time`: 起始时间过滤 (基于 created_at 字段，格式: "2025-01-01 00:00:00" 或 "2025-01-01T00:00:00")
- `end_time`: 结束时间过滤 (基于 created_at 字段，格式: "2025-01-31 23:59:59" 或 "2025-01-31T23:59:59")
- `limit`: 限制返回数量 (默认100, 最大1000)
- `duration`: 时间范围 (默认24h，当前未使用)

**参数优先级**: 
1. `start_time` + `end_time` (时间范围查询，最高优先级)
2. `status` (状态过滤)
3. `severity` (严重程度过滤)
4. `alert_type` (告警类型过滤)
5. `host_ip` (主机IP过滤)
6. `box_id` (机箱号过滤)
7. `slot_id` (板卡号过滤)
8. 默认：返回除Pending外的所有告警（Firing和Resolved）

**组合查询说明**: 
- 当使用 `start_time` 和 `end_time` 进行时间范围查询时，可以在内存中同时应用其他过滤条件（status、severity、alert_type、host_ip、box_id、slot_id）
- 其他过滤参数之间互斥，按优先级顺序使用第一个匹配的参数

**示例**:
```bash
# 通过机箱号查询
GET /alarm/events?box_id=1

# 通过板卡号查询
GET /alarm/events?slot_id=2

# 通过时间范围查询
GET /alarm/events?start_time=2025-01-01 00:00:00&end_time=2025-01-31 23:59:59

# 组合查询：时间范围 + 机箱号 + 状态
GET /alarm/events?start_time=2025-01-01 00:00:00&end_time=2025-01-31 23:59:59&box_id=1&status=firing

# 组合查询：时间范围 + 板卡号 + 严重程度
GET /alarm/events?start_time=2025-01-01 00:00:00&end_time=2025-01-31 23:59:59&slot_id=2&severity=严重
```

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": [
    {
      "id": "<告警ID>",
      "fingerprint": "<告警指纹>",
      "status": "pending|firing|resolved",
      "created_at": "<创建时间>",
      "updated_at": "<更新时间>",
      "starts_at": "<开始时间>",
      "ends_at": "<结束时间>",
      "labels": {
        "alert_name": "<告警名称>",
        "host_ip": "<主机IP>",
        "box_id": "<机箱号>",
        "slot_id": "<板卡号>",
        ...
      },
      "annotations": {
        "summary": "<摘要>",
        "description": "<描述>",
        ...
      }
    }
  ]
}
```

---

### GET /alarm/events/{id}
**功能**: 获取特定告警事件详情

**路径参数**:
- `id`: 告警事件ID

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "<告警ID>",
    "fingerprint": "<告警指纹>",
    "status": "pending|firing|resolved",
    "created_at": "<创建时间>",
    "updated_at": "<更新时间>",
    "starts_at": "<开始时间>",
    "ends_at": "<结束时间>",
    "labels": {...},
    "annotations": {...}
  }
}
```

**错误响应** (告警不存在):
```json
{
  "api_version": 2,
  "status": "error",
  "message": "alert not found",
  "data": {}
}
```

---

## 告警统计 (Alert Statistics)

### GET /alarm/count
**功能**: 获取告警总数

**参数**: 无

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "count": <告警总数>
  }
}
```

---

## 组件告警上报 (Component Alert)

### POST /alert/component
**功能**: 组件状态告警上报

**请求体格式**:
```json
{
  "host_ip": "<主机IP>",
  "instance_id": "<实例ID>",
  "uuid": "<UUID>",
  "index": "<索引>",
  "status": "<状态>"
}
```

**必需字段**:
- `host_ip`: 主机IP地址
- `instance_id`: 组件实例ID

**可选字段**:
- `uuid`: 组件UUID
- `index`: 组件索引
- `status`: 组件状态 (默认 "unknown")

**响应格式**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "<告警ID>",
    "fingerprint": "<告警指纹>",
    "message": "Component alert created successfully"
  }
}
```

---

## 路由统计

| 路由类型 | 数量 | 说明 |
|---------|------|------|
| 告警规则管理 | 5 | 创建、查询、更新、删除规则 |
| 告警事件查询 | 2 | 查询告警事件列表和详情 |
| 告警统计 | 1 | 获取告警总数 |
| 组件告警上报 | 1 | 接收组件状态告警 |

**总计**: 9个路由

---

## 通用说明

### API版本
所有路由的响应都包含 `api_version` 字段，当前版本为 `2`

### 错误响应格式
所有错误响应统一格式：
```json
{
  "api_version": 2,
  "status": "error",
  "message": "<错误信息>",
  "data": {}
}
```

### HTTP状态码
- `200 OK`: 成功
- `400 Bad Request`: 请求参数错误
- `404 Not Found`: 资源不存在
- `500 Internal Server Error`: 服务器内部错误

### 内容类型
所有响应内容类型为 `application/json`

### 告警状态
告警状态支持以下值：
- `pending`: 匹配但还未满足持续时间条件
- `firing`: 触发告警
- `resolved`: 告警已解决

