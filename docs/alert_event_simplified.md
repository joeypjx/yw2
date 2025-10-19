# AlertEvent 结构精简总结

## 已完成的工作

### 1. AlertEvent 结构重构 ✅

**参考:** `UserAlertEventView` (src/web/dto/alert_dto.h)

**重构前的 AlertEvent 字段：**
```cpp
struct AlertEvent {
    std::int64_t timestamp_ms;
    std::int64_t resolved_timestamp_ms;
    std::string fingerprint;
    std::string rule_id;
    std::string action;
    AlertStatus status;           // 枚举类型
    Severity severity;            // 枚举类型
    LabelSet labels;
    std::string title;
    std::string description;
    double value;
    std::string unit;
    nlohmann::json context;
};
```

**重构后的 AlertEvent 字段（精简版）：**
```cpp
struct AlertEvent {
    std::string fingerprint;      // 告警指纹（唯一标识）
    LabelSet labels;              // 标签集合
    std::string status;           // 状态字符串: inactive/pending/firing/resolved
    std::string summary;          // 告警摘要
    std::string description;      // 告警描述
    std::string starts_at;        // 开始时间（ISO格式字符串）
    std::string ends_at;          // 结束时间（ISO格式字符串，可为空）
    std::string created_at;       // 创建时间
    std::string updated_at;       // 更新时间
};
```

**移除的字段：**
- `timestamp_ms` / `resolved_timestamp_ms` → 改为 `starts_at` / `ends_at` (字符串格式)
- `rule_id` → 移除（不在事件中存储规则ID）
- `action` → 移除（通过 status 字段表达）
- `status` (枚举) → 改为字符串类型
- `severity` → 移除（从 Rule 中获取）
- `title` → 改名为 `summary`
- `value`, `unit`, `context` → 移除（简化数据结构）

### 2. JSON 序列化更新 ✅

```cpp
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AlertEvent,
    fingerprint, labels, status, summary, description,
    starts_at, ends_at, created_at, updated_at)
```

### 3. 数据库表结构创建 ✅

**文件:** `/Users/panjinxue/编程/yw2/docs/alert_event_setup_v2.sql`

**表结构：**
```sql
CREATE TABLE IF NOT EXISTS alert_event (
    fingerprint     TEXT NOT NULL,
    labels          JSONB NOT NULL,
    status          TEXT NOT NULL,
    summary         TEXT NOT NULL DEFAULT '',
    description     TEXT NOT NULL DEFAULT '',
    starts_at       TIMESTAMPTZ NOT NULL,
    ends_at         TIMESTAMPTZ,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (fingerprint, starts_at)
);
```

**索引：**
- status + created_at
- fingerprint + starts_at
- starts_at (降序)
- labels (GIN索引，用于JSONB查询)

**视图：**
- `active_alert_events` - 活跃告警（pending/firing状态）
- `recent_alert_events` - 最近24小时的告警
- `alert_event_statistics` - 按状态统计（最近7天）

### 4. DatabaseEventRepository 重写 ✅

**更新内容：**

**append() 方法：**
- 使用 `fingerprint + starts_at` 作为复合主键
- 检查记录是否存在，存在则UPDATE，不存在则INSERT
- `ends_at` 为空时使用 `pqxx::null`

**query() 方法：**
- 查询所有新字段
- 返回字符串格式的时间（不再转换为毫秒）
- 解析 JSONB labels

**countByStatus() 方法：**
- status 从枚举转换为字符串进行查询

## JSON 格式示例

**AlertEvent JSON 格式：**
```json
{
  "fingerprint": "cpu_high|host_ip=192.168.10.20",
  "labels": {
    "host_ip": "192.168.10.20",
    "host_type": "server"
  },
  "status": "firing",
  "summary": "CPU使用率超过80%",
  "description": "主机192.168.10.20的CPU使用率为85.6%，持续超过5分钟",
  "starts_at": "2025-01-15T10:30:00Z",
  "ends_at": "",
  "created_at": "2025-01-15T10:30:00Z",
  "updated_at": "2025-01-15T10:35:00Z"
}
```

## 与 UserAlertEventView 的对应关系

| AlertEvent字段 | UserAlertEventView字段 | 说明 |
|---------------|----------------------|------|
| fingerprint | fingerprint / id | 唯一标识 |
| labels | labels | 标签集合 |
| status | status | 状态字符串 |
| summary | annotations.summary | 告警摘要 |
| description | annotations.description | 告警描述 |
| starts_at | starts_at | 开始时间 |
| ends_at | ends_at | 结束时间 |
| created_at | created_at | 创建时间 |
| updated_at | updated_at | 更新时间 |

## 字段变化对比

| 旧字段 | 新字段 | 变化说明 |
|-------|-------|---------|
| timestamp_ms (int64) | starts_at (string) | 时间戳改为ISO字符串 |
| resolved_timestamp_ms (int64) | ends_at (string) | 时间戳改为ISO字符串 |
| title | summary | 改名 |
| status (枚举) | status (string) | 类型改变 |
| rule_id | **已删除** | 不再存储规则ID |
| action | **已删除** | 通过status表达 |
| severity | **已删除** | 从Rule中获取 |
| value | **已删除** | 简化数据 |
| unit | **已删除** | 简化数据 |
| context | **已删除** | 简化数据 |

## 迁移建议

### 1. 数据库迁移

```sql
-- 删除旧表（谨慎操作，请先备份数据）
DROP TABLE IF EXISTS alert_event CASCADE;

-- 创建新表
-- 执行 docs/alert_event_setup_v2.sql
```

### 2. 数据转换示例（旧 → 新）

**旧格式：**
```json
{
  "timestamp_ms": 1705315800000,
  "fingerprint": "cpu_high|host_ip=192.168.10.20",
  "rule_id": "cpu_high",
  "action": "firing",
  "status": "firing",
  "severity": "warn",
  "title": "CPU使用率过高",
  "value": 85.6
}
```

**新格式：**
```json
{
  "fingerprint": "cpu_high|host_ip=192.168.10.20",
  "status": "firing",
  "summary": "CPU使用率过高",
  "description": "主机192.168.10.20的CPU使用率为85.6%",
  "starts_at": "2025-01-15T10:30:00Z",
  "ends_at": ""
}
```

## 注意事项

### 1. 时间格式
- 使用 ISO8601 格式字符串：`2025-01-15T10:30:00Z`
- PostgreSQL 会自动解析标准时间格式
- 前端显示时需要格式化

### 2. ends_at 处理
- firing 状态时 `ends_at` 为空字符串或 NULL
- resolved 状态时需要设置 `ends_at`

### 3. 复合主键
- 使用 `(fingerprint, starts_at)` 作为主键
- 同一个告警在不同时间段会有多条记录
- 允许告警恢复后再次触发

### 4. labels 查询
- 使用 GIN 索引优化 JSONB 查询
- 查询示例：
  ```sql
  SELECT * FROM alert_event
  WHERE labels @> '{"host_ip": "192.168.10.20"}'::jsonb;
  ```

## 文件清单

### 已修改的文件：
1. `/Users/panjinxue/编程/yw2/include/yw/alert_model.h` - AlertEvent结构定义和JSON序列化
2. `/Users/panjinxue/编程/yw2/src/alert/DatabaseEventRepository.cpp` - 数据库操作实现

### 已创建的文件：
1. `/Users/panjinxue/编程/yw2/docs/alert_event_setup_v2.sql` - 数据库表结构
2. `/Users/panjinxue/编程/yw2/docs/alert_event_simplified.md` - 本文档

## 后续工作

### 必须完成：
1. 更新 AlertStateManager 中生成 AlertEvent 的代码
2. 确保时间格式转换正确（毫秒时间戳 → ISO字符串）
3. 更新相关的 API 路由和控制器

### 可选改进：
1. 添加告警事件的聚合查询
2. 实现告警事件的统计分析
3. 添加单元测试
