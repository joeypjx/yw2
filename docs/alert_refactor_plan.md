# 告警规则评估逻辑重构方案

## 一、重构概述

本次重构将告警规则的数据结构从简单的字符串表达式改为结构化的条件表达式模型，支持更灵活的告警配置和评估逻辑。

## 二、新的数据结构

### 1. Condition (条件)
```cpp
struct Condition {
    std::string op;         // 操作符: >, <, >=, <=, ==, !=
    double threshold;       // 阈值
};
```

### 2. Expression (表达式)
```cpp
struct Expression {
    std::string stable;                 // 资源名称: cpu, memory, disk, network, gpu
    std::string metric;                 // 指标名称: usage_percent, load_avg_1m等
    std::vector<Condition> conditions;  // 条件列表（所有条件需同时满足）
    std::vector<LabelSet> tags;         // 标签匹配列表（用于过滤目标节点）
};
```

### 3. Rule (告警规则)
```cpp
struct Rule {
    std::string id;
    std::string alert_name;      // 告警规则名称
    Expression expression;       // 表达式配置
    std::string for_duration;    // 持续时间，如 30s, 5m
    Severity severity;           // 严重等级
    std::string summary;         // 告警摘要
    std::string description;     // 告警详细描述模板（支持变量替换）
    std::string alert_type;      // 告警类型: resource

    // 兼容字段
    std::string eval_every;      // 评估频率
    bool enabled;
    std::string created_at;
    std::string updated_at;
    std::int32_t for_times;      // 运行时计算
};
```

## 三、JSON 格式示例

```json
{
  "id": "cpu_high_alert",
  "alert_name": "CPU使用率过高告警",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {
        "operator": ">",
        "threshold": 80
      }
    ],
    "tags": [
      {
        "host_ip": "192.168.10.20"
      }
    ]
  },
  "for": "30s",
  "severity": "一般",
  "summary": "主机{{host_ip}}的CPU使用率超过80%",
  "description": "主机{{host_ip}}的CPU使用率为{{value}}%，持续超过30秒",
  "alert_type": "resource",
  "eval_every": "10s",
  "enabled": true
}
```

## 四、核心评估逻辑

### 1. 持续时间判断
- `for_duration` 指定了告警触发前需要满足条件的持续时间
- 评估器查询 `for_duration` 时间窗口内的所有数据点
- 只有当**所有数据点**都满足**所有条件**时，才判定为匹配

### 2. 标签过滤 (tags)
- `tags` 是一个标签集合的数组
- 每个标签集合内的条件是 **AND** 关系（所有标签都要匹配）
- 多个标签集合之间是 **OR** 关系（匹配任意一个即可）
- 示例：
  ```json
  "tags": [
    {"host_ip": "192.168.10.20", "host_type": "server"},
    {"host_ip": "192.168.10.21"}
  ]
  ```
  表示：匹配 (host_ip=192.168.10.20 AND host_type=server) 或 (host_ip=192.168.10.21)

### 3. 条件判断 (conditions)
- 所有条件都必须同时满足（AND 关系）
- 支持的操作符：`>`, `<`, `>=`, `<=`, `==`, `!=`
- 示例：
  ```json
  "conditions": [
    {"operator": ">=", "threshold": 80},
    {"operator": "<", "threshold": 100}
  ]
  ```
  表示：80 <= value < 100

## 五、资源表映射

| stable   | 数据表名        | 常用指标 (metric)                          |
|----------|---------------|-----------------------------------------|
| cpu      | resource_cpu  | usage_percent, load_avg_1m, temperature |
| memory   | resource_memory | usage_percent, used, free             |
| disk     | resource_disk | usage_percent, used, free              |
| network  | resource_network | rx_rate, tx_rate, rx_errors          |
| gpu      | resource_gpu  | compute_usage, mem_usage, temperature  |
| alive    | resource_alive | alive (心跳)                           |

## 六、实现文件

### 已更新/创建的文件：
1. `/Users/panjinxue/编程/yw2/include/yw/alert_model.h` - 数据结构定义
2. `/Users/panjinxue/编程/yw2/src/alert/alert_services.h` - 接口定义
3. `/Users/panjinxue/编程/yw2/src/alert/AlertServices.h` - 服务类声明
4. `/Users/panjinxue/编程/yw2/src/alert/AlertEvaluatorV2.cpp` - 新的评估器实现

### 需要更新的文件：
1. `DatabaseRuleRepository.cpp` - 数据库规则仓库（需要适配新的字段）
2. `AlertManager.cpp` - 告警管理器（需要更新规则加载和for_times计算）
3. `docs/alert_rule_setup.sql` - 数据库表结构（需要添加新字段）
4. `src/alert/CMakeLists.txt` - 构建配置（添加 AlertEvaluatorV2.cpp）

## 七、兼容性说明

### 字段映射（旧 -> 新）：
- `name` -> `alert_name`
- `expression` (字符串) -> `expression` (结构化对象)
- `window` -> `for_duration`
- `selector` -> `expression.tags[0]`
- `tag` -> `alert_type`

### JSON 反序列化兼容：
- 在 `from_json` 中添加了兼容逻辑
- 支持同时接受 `name` 和 `alert_name` 字段
- 旧的字符串 `expression` 将不再支持（需要迁移）

## 八、迁移建议

### 1. 数据库迁移
```sql
-- 添加新字段
ALTER TABLE alert_rule
ADD COLUMN IF NOT EXISTS alert_name TEXT,
ADD COLUMN IF NOT EXISTS summary TEXT,
ADD COLUMN IF NOT EXISTS for_duration TEXT,
ADD COLUMN IF NOT EXISTS alert_type TEXT DEFAULT 'resource',
ADD COLUMN IF NOT EXISTS expression_v2 JSONB;

-- 迁移现有数据（示例）
UPDATE alert_rule SET
    alert_name = name,
    summary = name,
    for_duration = time_window,
    alert_type = COALESCE(tag, 'resource');
```

### 2. 旧规则转换示例
旧格式：
```json
{
  "id": "cpu_high",
  "name": "CPU使用率过高",
  "expression": "cpu.usage_percent.avg > 80",
  "window": "5m",
  "selector": {"host_type": "server"},
  "for_times": 2
}
```

新格式：
```json
{
  "id": "cpu_high",
  "alert_name": "CPU使用率过高",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [{"operator": ">", "threshold": 80}],
    "tags": [{"host_type": "server"}]
  },
  "for": "5m",
  "eval_every": "30s",
  "for_times": 2
}
```

## 九、使用示例

### 1. CPU 使用率告警
```json
{
  "alert_name": "CPU使用率告警",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">", "threshold": 80}
    ],
    "tags": [{"host_ip": "192.168.10.20"}]
  },
  "for": "30s",
  "severity": "一般",
  "summary": "CPU使用率过高",
  "description": "主机{{host_ip}}的CPU使用率为{{value}}%"
}
```

### 2. 内存使用率区间告警
```json
{
  "alert_name": "内存使用率异常",
  "expression": {
    "stable": "memory",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">=", "threshold": 70},
      {"operator": "<", "threshold": 90}
    ],
    "tags": []
  },
  "for": "1m",
  "severity": "一般"
}
```

### 3. 节点离线告警
```json
{
  "alert_name": "节点离线",
  "expression": {
    "stable": "alive",
    "metric": "alive",
    "conditions": [
      {"operator": "==", "threshold": 0}
    ],
    "tags": [{"host_ip": "192.168.10.20"}]
  },
  "for": "5s",
  "severity": "严重",
  "summary": "节点{{host_ip}}离线",
  "description": "节点{{host_ip}}心跳超时，判定为离线状态"
}
```

## 十、后续工作

1. 完成 DatabaseRuleRepository 的字段适配
2. 更新 AlertManager 中的 for_times 计算逻辑
3. 更新数据库表结构 SQL 文件
4. 添加单元测试覆盖新的评估逻辑
5. 编写 API 文档说明新的数据格式
6. 提供规则迁移工具或脚本
