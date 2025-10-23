# AlertV2 API 接口文档

## 概述

AlertV2 API提供了完整的告警规则管理和告警事件查询功能，支持RESTful风格的接口设计。

## 基础信息

- **API版本**: v2
- **基础路径**: `/api/v2/alarm`
- **响应格式**: JSON
- **字符编码**: UTF-8

## 响应格式

所有API响应都遵循统一的格式：

```json
{
  "api_version": 2,
  "status": "success|error",
  "message": "描述信息",
  "data": {} // 具体数据
}
```

## 告警规则管理接口

### 1. 创建告警规则

**POST** `/api/v2/alarm/rules`

**请求体示例**:
```json
{
  "alert_name": "CPU使用率过高",
  "alert_type": "硬件资源",
  "severity": "严重",
  "summary": "CPU使用率超过阈值",
  "description": "节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过阈值",
  "for": "5s",
  "enabled": true,
  "expression": {
    "stable": "cpu",
    "metric": "usage",
    "conditions": [
      {
        "operator": ">",
        "value": 80
      }
    ],
    "tags": []
  }
}
```

**响应示例**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "id": "rule_20251024_014022_523_4658",
    "message": "Rule created successfully"
  }
}
```

### 2. 获取所有告警规则

**GET** `/api/v2/alarm/rules`

**响应示例**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": [
    {
      "id": "rule_20251024_014022_523_4658",
      "alert_name": "CPU使用率过高",
      "alert_type": "硬件资源",
      "severity": "严重",
      "summary": "CPU使用率超过阈值",
      "description": "节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过阈值",
      "for": "5s",
      "enabled": true,
      "expression": {
        "stable": "cpu",
        "metric": "usage",
        "conditions": [
          {
            "operator": ">",
            "value": 80
          }
        ],
        "tags": []
      },
      "created_at": "2025-10-24T01:40:22.523Z",
      "updated_at": "2025-10-24T01:40:22.523Z"
    }
  ]
}
```

### 3. 获取特定告警规则

**GET** `/api/v2/alarm/rules/{id}`

**路径参数**:
- `id`: 告警规则ID

### 4. 更新告警规则

**PUT** `/api/v2/alarm/rules/{id}`

**请求体**: 与创建接口相同

### 5. 删除告警规则

**POST** `/api/v2/alarm/rules/{id}/delete`

### 6. 获取启用的告警规则

**GET** `/api/v2/alarm/rules/enabled`

### 7. 启用告警规则

**POST** `/api/v2/alarm/rules/{id}/enable`

### 8. 禁用告警规则

**POST** `/api/v2/alarm/rules/{id}/disable`

## 告警事件查询接口

### 1. 获取告警事件

**GET** `/api/v2/alarm/events`

**查询参数**:
- `duration`: 时间范围（默认: "24h"）
- `status`: 状态过滤（pending|firing|resolved）
- `severity`: 严重程度过滤
- `alert_type`: 告警类型过滤
- `host_ip`: 主机IP过滤
- `limit`: 限制数量（默认: 100，最大: 1000）

**请求示例**:
```
GET /api/v2/alarm/events?status=firing&limit=10
GET /api/v2/alarm/events?severity=严重
GET /api/v2/alarm/events?host_ip=192.168.1.100
```

**响应示例**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": [
    {
      "id": "alert_20251024_014054_293_5536",
      "fingerprint": "测试告警|host_ip=192.168.1.100",
      "status": "firing",
      "created_at": "2025-10-23T17:40:54.293+00",
      "updated_at": "2025-10-23T17:40:54.376178+00",
      "starts_at": "2025-10-23T17:40:54.343+00",
      "ends_at": "",
      "labels": {
        "alert_name": "测试告警",
        "alert_type": "测试类型",
        "host_ip": "192.168.1.100",
        "metric": "test_metric",
        "severity": "一般",
        "value": "100.0"
      },
      "annotations": {
        "summary": "这是一个测试告警",
        "description": "用于演示Alert.updateInDatabase方法的功能"
      }
    }
  ]
}
```

### 2. 获取特定告警事件

**GET** `/api/v2/alarm/events/{id}`

**路径参数**:
- `id`: 告警事件ID

### 3. 获取告警统计

**GET** `/api/v2/alarm/count`

**查询参数**:
- `status`: 状态过滤
- `severity`: 严重程度过滤
- `alert_type`: 告警类型过滤

**请求示例**:
```
GET /api/v2/alarm/count?status=firing
GET /api/v2/alarm/count?severity=严重
```

**响应示例**:
```json
{
  "api_version": 2,
  "status": "success",
  "data": {
    "count": 11
  }
}
```

## 错误处理

### HTTP状态码

- `200`: 成功
- `400`: 请求参数错误
- `404`: 资源不存在
- `500`: 服务器内部错误

### 错误响应示例

```json
{
  "api_version": 2,
  "status": "error",
  "message": "rule not found",
  "data": {}
}
```

## 使用示例

### Python示例

```python
import requests

# 获取所有告警事件
response = requests.get("http://localhost:8080/api/v2/alarm/events")
data = response.json()
print(f"告警事件数量: {len(data['data'])}")

# 按状态过滤
response = requests.get("http://localhost:8080/api/v2/alarm/events?status=firing")
data = response.json()
print(f"Firing状态告警数量: {len(data['data'])}")

# 创建告警规则
rule_data = {
    "alert_name": "内存使用率过高",
    "alert_type": "硬件资源",
    "severity": "一般",
    "summary": "内存使用率超过阈值",
    "description": "节点 {{host_ip}} 的内存使用率为 {{value}}%，超过阈值",
    "for": "10s",
    "enabled": True,
    "expression": {
        "stable": "memory",
        "metric": "usage",
        "conditions": [{"operator": ">", "value": 90}],
        "tags": []
    }
}
response = requests.post("http://localhost:8080/api/v2/alarm/rules", json=rule_data)
print(f"创建结果: {response.json()}")
```

### curl示例

```bash
# 获取所有告警事件
curl -X GET "http://localhost:8080/api/v2/alarm/events"

# 按状态过滤
curl -X GET "http://localhost:8080/api/v2/alarm/events?status=firing&limit=10"

# 获取告警统计
curl -X GET "http://localhost:8080/api/v2/alarm/count?status=firing"

# 创建告警规则
curl -X POST "http://localhost:8080/api/v2/alarm/rules" \
  -H "Content-Type: application/json" \
  -d '{
    "alert_name": "CPU使用率过高",
    "alert_type": "硬件资源",
    "severity": "严重",
    "summary": "CPU使用率超过阈值",
    "description": "节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过阈值",
    "for": "5s",
    "enabled": true,
    "expression": {
      "stable": "cpu",
      "metric": "usage",
      "conditions": [{"operator": ">", "value": 80}],
      "tags": []
    }
  }'
```

## 注意事项

1. **API版本**: 所有接口都使用v2版本，响应中包含`api_version: 2`
2. **时间格式**: 时间字段使用ISO 8601格式
3. **状态值**: 告警状态为`pending`、`firing`、`resolved`
4. **限制**: 单次查询最多返回1000条记录
5. **过滤**: 支持多种过滤条件，可以组合使用
6. **错误处理**: 所有错误都会返回统一的错误格式

## 与v1 API的区别

| 特性 | v1 API | v2 API |
|------|--------|--------|
| 路径前缀 | `/alarm` | `/api/v2/alarm` |
| 响应格式 | 不统一 | 统一格式 |
| 告警规则 | 基础功能 | 完整CRUD + 启用/禁用 |
| 告警事件 | 基础查询 | 多维度过滤查询 |
| 统计功能 | 基础统计 | 增强统计功能 |
| 错误处理 | 简单 | 详细错误信息 |
