# 告警规则 SQL 转换文档

本文档详细说明告警规则（AlertRule）如何转换为数据库 SQL 查询语句的过程。

## 概述

告警规则通过 `AlertRuleEvaluator::convertRuleToSQL()` 方法转换为 SQL 查询语句，用于从 TimescaleDB 时序数据库中查询满足告警条件的数据。

## 告警规则结构

### AlertRule 结构

```cpp
class AlertRule {
    std::string alert_name_;      // 告警规则标识
    AlertExpression expression_;   // 告警表达式（核心部分）
    std::string for_;             // 持续时间（如 "5s", "1m", "1h"）
    std::string severity_;        // 告警等级
    std::string summary_;         // 告警摘要
    std::string description_;     // 告警详情（支持占位符 {{}}）
    std::string alert_type_;      // 告警类型
    bool enabled_;                // 是否启用
};
```

### AlertExpression 结构

```cpp
struct AlertExpression {
    std::string stable;                           // 资源类型: cpu, memory, disk, network, gpu, alive
    std::string metric;                          // 指标名称（如 usage_percent, total, alive）
    std::vector<AlertCondition> conditions;      // 条件列表（所有条件需同时满足，AND关系）
    std::vector<std::unordered_map<std::string, std::string>> tags;  // 标签匹配列表（OR关系）
};
```

### AlertCondition 结构

```cpp
struct AlertCondition {
    std::string operator_;    // 操作符: >, <, >=, <=, ==, !=
    double threshold;         // 阈值
};
```

## SQL 转换流程

### 1. 基本 SQL 结构

转换后的 SQL 使用 PostgreSQL 的 `DISTINCT ON` 语法，获取每个节点（和标签组合）的最新数据：

```sql
SELECT DISTINCT ON (host_ip, [tag_columns]) 
    host_ip, [tag_columns], [metric]
FROM [table_name]
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND [where_conditions]
ORDER BY host_ip, [tag_columns], time DESC
```

### 2. 转换步骤详解

#### 步骤 1: 确定数据表名

根据 `expression.stable` 映射到对应的 TimescaleDB 表：

| stable | 表名 |
|--------|------|
| `cpu` | `resource_cpu` |
| `memory` | `resource_memory` |
| `disk` | `resource_disk` |
| `network` | `resource_network` |
| `gpu` | `resource_gpu` |
| `alive` | `resource_alive` |

**代码位置**: `AlertRuleEvaluator::getTableName()`

#### 步骤 2: 确定标签列

根据 `stable` 类型确定需要查询的标签列（用于分组和过滤）：

| stable | 标签列 |
|--------|--------|
| `cpu` | 无 |
| `memory` | 无 |
| `disk` | `device`, `mount_point` |
| `network` | `interface` |
| `gpu` | `gpu_index` |
| `alive` | 无 |

**代码位置**: `AlertRuleEvaluator::getTagColumns()`

#### 步骤 3: 构建 SELECT 子句

```cpp
sql << "SELECT DISTINCT ON (host_ip" << tagColumnsList.str() << ") ";
sql << "host_ip" << tagColumnsList.str() << ", " << expr.metric;
```

**说明**:
- `DISTINCT ON (host_ip, [tag_columns])` 确保每个节点和标签组合只返回一条最新记录
- 查询字段包括：`host_ip`、标签列（如果有）、指标列

#### 步骤 4: 构建 WHERE 子句

WHERE 子句包含两部分：

1. **时间范围条件**（固定）:
   ```sql
   time >= NOW() - INTERVAL '10 seconds'
   ```
   查询最近 10 秒的数据

2. **标签条件**（来自 `expression.tags`）:
   - 多个标签组之间是 **OR** 关系
   - 同一标签组内的多个标签是 **AND** 关系
   - 格式：`(tag1 = 'value1' AND tag2 = 'value2') OR (tag3 = 'value3')`

3. **指标条件**（来自 `expression.conditions`）:
   - 所有条件之间是 **AND** 关系
   - 格式：`metric > threshold AND metric < threshold2`
   - 操作符转换：`==` → `=`, `!=` → `<>`
   - 特殊处理：`alive` 字段使用 `::integer` 类型转换

**代码位置**: 
- `AlertRuleEvaluator::buildWhereConditions()` - 主入口
- `AlertRuleEvaluator::buildTagConditions()` - 构建标签条件
- `AlertRuleEvaluator::buildMetricConditions()` - 构建指标条件

#### 步骤 5: 构建 ORDER BY 子句

```sql
ORDER BY host_ip, [tag_columns], time DESC
```

确保 `DISTINCT ON` 返回的是每个组合的最新记录（按时间降序）。

## 转换示例

### 示例 1: CPU 使用率告警

**告警规则 JSON**:
```json
{
  "alert_name": "CPU使用率过高",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">", "threshold": 80.0}
    ],
    "tags": []
  }
}
```

**转换后的 SQL**:
```sql
SELECT DISTINCT ON (host_ip) 
    host_ip, usage_percent
FROM resource_cpu
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND usage_percent > 80.0::numeric
ORDER BY host_ip, time DESC
```

### 示例 2: 磁盘使用率告警（带标签过滤）

**告警规则 JSON**:
```json
{
  "alert_name": "数据盘使用率过高",
  "expression": {
    "stable": "disk",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">", "threshold": 85.0}
    ],
    "tags": [
      {"mount_point": "/data"},
      {"mount_point": "/var"}
    ]
  }
}
```

**转换后的 SQL**:
```sql
SELECT DISTINCT ON (host_ip, device, mount_point) 
    host_ip, device, mount_point, usage_percent
FROM resource_disk
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND (mount_point = '/data' OR mount_point = '/var')
  AND usage_percent > 85.0::numeric
ORDER BY host_ip, device, mount_point, time DESC
```

### 示例 3: 网络接口告警（多标签组合）

**告警规则 JSON**:
```json
{
  "alert_name": "网络接口错误率高",
  "expression": {
    "stable": "network",
    "metric": "rx_errors",
    "conditions": [
      {"operator": ">", "threshold": 100.0}
    ],
    "tags": [
      {"interface": "eth0"},
      {"interface": "eth1", "host_ip": "192.168.1.5"}
    ]
  }
}
```

**转换后的 SQL**:
```sql
SELECT DISTINCT ON (host_ip, interface) 
    host_ip, interface, rx_errors
FROM resource_network
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND (interface = 'eth0' OR (interface = 'eth1' AND host_ip = '192.168.1.5'))
  AND rx_errors > 100.0::numeric
ORDER BY host_ip, interface, time DESC
```

### 示例 4: 节点存活告警（特殊类型处理）

**告警规则 JSON**:
```json
{
  "alert_name": "节点离线",
  "expression": {
    "stable": "alive",
    "metric": "alive",
    "conditions": [
      {"operator": "==", "threshold": 0.0}
    ],
    "tags": []
  }
}
```

**转换后的 SQL**:
```sql
SELECT DISTINCT ON (host_ip) 
    host_ip, alive
FROM resource_alive
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND alive::integer = 0
ORDER BY host_ip, time DESC
```

**注意**: `alive` 字段使用 `::integer` 类型转换，其他指标使用 `::numeric`。

### 示例 5: 多条件告警

**告警规则 JSON**:
```json
{
  "alert_name": "内存使用率在范围内",
  "expression": {
    "stable": "memory",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">", "threshold": 70.0},
      {"operator": "<", "threshold": 90.0}
    ],
    "tags": []
  }
}
```

**转换后的 SQL**:
```sql
SELECT DISTINCT ON (host_ip) 
    host_ip, usage_percent
FROM resource_memory
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND usage_percent > 70.0::numeric
  AND usage_percent < 90.0::numeric
ORDER BY host_ip, time DESC
```

## 关键实现细节

### 1. 标签条件的 OR 逻辑

```cpp
// tags 数组中的每个元素是一个标签组（AND关系）
// 不同标签组之间是 OR 关系
for (const auto& tagGroup : tags) {
    if (!first) {
        conditions << " OR ";  // 标签组之间用 OR 连接
    }
    conditions << "(";
    
    // 同一标签组内的标签用 AND 连接
    for (const auto& tag : tagGroup) {
        if (!firstTag) {
            conditions << " AND ";
        }
        conditions << tag.first << " = '" << tag.second << "'";
    }
    conditions << ")";
}
```

### 2. 指标条件的 AND 逻辑

```cpp
// 所有 conditions 之间用 AND 连接
for (const auto& condition : conditions) {
    if (!first) {
        sql << " AND ";
    }
    // 转换操作符
    std::string op = condition.operator_;
    if (op == "==") op = "=";
    if (op == "!=") op = "<>";
    
    // 特殊处理 alive 字段
    if (metric == "alive") {
        sql << metric << "::integer " << op << " " << static_cast<int>(condition.threshold);
    } else {
        sql << metric << " " << op << " " << condition.threshold << "::numeric";
    }
}
```

### 3. 时间窗口

所有查询都限制在最近 10 秒的数据：
```sql
WHERE time >= NOW() - INTERVAL '10 seconds'
```

这确保了只查询最新的数据，避免处理过时的数据。

### 4. DISTINCT ON 的使用

`DISTINCT ON` 确保每个 `(host_ip, [tag_columns])` 组合只返回一条记录（最新的）：

```sql
SELECT DISTINCT ON (host_ip, device, mount_point) 
    ...
ORDER BY host_ip, device, mount_point, time DESC
```

**注意**: `DISTINCT ON` 要求 ORDER BY 的第一个表达式必须与 DISTINCT ON 的表达式匹配。

## 查询结果处理

SQL 查询执行后，结果通过 `convertQueryResultToAlerts()` 方法处理：

1. **条件检查**: 使用 `checkAlertConditions()` 在应用层再次验证条件（双重检查）
2. **标签构建**: 从查询结果构建告警标签（包括通过 IPAddressUtils 解析的 box_id 和 slot_id）
3. **告警创建**: 根据 `for` 字段决定初始状态（Pending 或 Firing）

## 注意事项

1. **SQL 注入防护**: 
   - 标签值直接拼接到 SQL 中，存在 SQL 注入风险
   - 建议：使用参数化查询（`$1`, `$2` 等占位符）

2. **性能考虑**:
   - 时间窗口固定为 10 秒，可能不适合所有场景
   - `DISTINCT ON` 需要排序，可能影响性能
   - 建议：根据实际需求调整时间窗口，添加索引优化

3. **类型转换**:
   - `alive` 字段使用 `::integer`
   - 其他指标使用 `::numeric`
   - 确保数据库字段类型与转换一致

4. **标签匹配**:
   - 标签值使用单引号包裹，如果值中包含单引号会导致 SQL 错误
   - 建议：转义特殊字符或使用参数化查询

## 改进建议

1. **使用参数化查询**: 将标签值和阈值作为参数传递，避免 SQL 注入
2. **可配置时间窗口**: 允许规则配置查询时间窗口，而不是固定 10 秒
3. **查询优化**: 为常用查询添加索引，优化 DISTINCT ON 性能
4. **错误处理**: 增强 SQL 构建过程的错误处理和验证

