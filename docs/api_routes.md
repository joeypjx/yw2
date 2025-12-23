# API Routes 文档

本文档记录了所有已注册的 HTTP API 路由。

## NodeRoutes (节点相关路由)

### GET /node
**功能**: 获取节点信息

**参数**:
- `host_ip` (可选, 查询参数): 返回指定IP的单个节点对象
- `box_id` (可选, 查询参数): 返回指定box_id的节点列表
- 无参数: 返回所有节点列表

**响应格式**:
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes": [...] // 或单个节点对象
  }
}
```

---

### GET /node/export
**功能**: 导出节点历史数据

**必需参数**:
- `ip` (查询参数): 节点IP地址
- `start_time` (查询参数): 开始时间戳
- `end_time` (查询参数): 结束时间戳

**可选参数**:
- `type` (查询参数): 指标类型，逗号分隔（如 "cpu,memory"），支持 "system" 表示所有类型

**响应格式**:
```json
{
  "code": 200,
  "data": [
    {
      "start_time": <timestamp>,
      "end_time": <timestamp>,
      "ip": "<ip_address>",
      "type": "<type>",
      "data": [...]
    }
  ]
}
```

---

## MetricsRoutes (指标相关路由)

### GET /node/metrics
**功能**: 获取所有节点的实时指标数据

**参数**: 无

**响应格式**:
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "nodes_metrics": [...]
  },
  "warnings": {  // 可选，如果有节点处理失败
    "failed_nodes_count": <count>
  }
}
```

---

### GET /node/historical-metrics
**功能**: 查询节点历史指标数据

**必需参数**:
- `host_ip` (查询参数): 节点IP地址

**可选参数**:
- `time_range` (查询参数): 时间范围，默认 "1m"
- `metrics` (查询参数): 指标类型列表，逗号分隔

**响应格式**:
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "historical_metrics": {...}
  }
}
```

---

## BMCRoutes (BMC相关路由)

### GET /box/bmc
**功能**: 获取机箱的BMC信息

**必需参数**:
- `box_id` (路径参数或查询参数): 机箱ID

**可选参数**:
- `duration` (查询参数): 时间范围，默认 "5m"

**响应格式**:
```json
{
  "api_version": 1,
  "status": "success",
  "data": {
    "box_id": <int>,
    "fan_0_speed": <int>,
    "fan_1_speed": <int>,
    "sensor": {...}
  }
}
```

---

### POST /box/bmc/fan_speed
**状态**: ⚠️ **已注释，当前不可用**

**功能**: 设置风扇速度

**说明**: 该路由的代码已被注释（第63-126行），因为IPMI模块暂时不编译。

**请求体格式**:
```json
{
  "box_id": <int>,
  "fan_speed": <int>
}
```

---

## 路由统计

| 路由文件 | 路由数量 | 状态 |
|---------|---------|------|
| NodeRoutes | 2 | ✅ 全部可用 |
| MetricsRoutes | 2 | ✅ 全部可用 |
| BMCRoutes | 1 | ✅ 1个可用，1个已注释 |

**总计**: 5个可用路由，1个已注释路由

---

## 注意事项

1. 所有路由的响应都包含 `api_version` 字段，当前版本为 `1`
2. 错误响应格式统一为：
   ```json
   {
     "api_version": 1,
     "status": "error",
     "message": "<error_message>",
     "data": {}
   }
   ```
3. 所有响应内容类型为 `application/json`
4. POST /box/bmc/fan_speed 路由需要IPMI模块支持，当前不可用

