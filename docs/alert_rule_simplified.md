# 告警规则结构精简总结

## 已完成的工作

### 1. Rule 结构精简 ✅

**精简前的字段：**
- id, alert_name, expression, for_duration, severity, summary, description, alert_type
- eval_every (已删除)
- enabled (已删除)
- for_times (已删除)
- created_at, updated_at

**精简后的字段（仅保留核心字段）：**
```cpp
struct Rule {
    std::string id;             // 规则ID
    std::string alert_name;     // 告警规则名称
    Expression expression;      // 表达式配置
    std::string for_duration;   // 持续时间(JSON中字段名为"for")
    Severity severity;          // 严重等级
    std::string summary;        // 告警摘要
    std::string description;    // 告警详细描述模板
    std::string alert_type;     // 告警类型
    std::string created_at;     // 创建时间
    std::string updated_at;     // 更新时间
};
```

### 2. JSON 序列化更新 ✅

精简了序列化代码，移除了不必要的字段:
- 移除 `eval_every`
- 移除 `enabled`
- 移除向后兼容逻辑（`name` -> `alert_name`）

### 3. 数据库表结构更新 ✅

**文件:** `/Users/panjinxue/编程/yw2/docs/alert_rule_setup_v2.sql`

**表结构（精简版）：**
```sql
CREATE TABLE IF NOT EXISTS alert_rule (
    id              TEXT PRIMARY KEY,
    alert_name      TEXT NOT NULL,
    expression      JSONB NOT NULL,
    for_duration    TEXT NOT NULL,
    severity        TEXT NOT NULL,
    summary         TEXT NOT NULL DEFAULT '',
    description     TEXT NOT NULL DEFAULT '',
    alert_type      TEXT NOT NULL DEFAULT 'resource',
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);
```

**移除的字段：**
- `eval_every` - 评估频率（不再需要）
- `enabled` - 是否启用标志（不再需要）

**保留的索引：**
- severity + created_at
- alert_type + created_at
- alert_name
- expression (GIN索引，用于JSONB查询)

### 4. DatabaseRuleRepository 更新 ✅

**更新内容：**
- `listRules()` - SQL 查询字段更新
- `getRule()` - SQL 查询字段更新
- `upsertRule()` - INSERT/UPDATE 字段更新，expression序列化为JSONB
- `ensureTableExists()` - 建表语句更新，索引更新
- `parseRuleFromRow()` - 解析JSONB expression字段

### 5. AlertEvaluatorV2 更新 ✅

**更新内容：**
- context 中移除 `eval_every` 字段
- 保留其他核心字段的映射

## JSON 格式示例

**最终的 JSON 格式：**
```json
{
  "id": "cpu_high",
  "alert_name": "CPU使用率过高",
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
        "host_type": "server"
      }
    ]
  },
  "for": "5m",
  "severity": "一般",
  "summary": "CPU使用率超过80%",
  "description": "主机{{host_ip}}的CPU使用率为{{value}}%，持续超过5分钟",
  "alert_type": "resource"
}
```

## 字段变化对比

| 旧字段名 | 新字段名 | 说明 |
|---------|---------|------|
| name | alert_name | 改名以更清晰 |
| expression (string) | expression (JSONB) | 从字符串改为结构化对象 |
| window | for_duration (JSON中为"for") | 改名更语义化 |
| selector | expression.tags | 合并到expression中 |
| tag | alert_type | 改名更清晰 |
| eval_every | **已删除** | 不再需要 |
| enabled | **已删除** | 不再需要 |
| for_times | **已删除** | 不再需要（由for_duration决定） |

## 迁移建议

### 1. 数据库迁移SQL

```sql
-- 删除旧表（谨慎操作，请先备份数据）
DROP TABLE IF EXISTS alert_rule CASCADE;

-- 创建新表
-- 执行 docs/alert_rule_setup_v2.sql
```

### 2. 代码迁移要点

1. **调度逻辑需要更新：**
   - 移除 `eval_every` 后，调度频率需要在 AlertManager 中独立配置
   - 或者为所有规则使用统一的评估频率

2. **规则启用/禁用：**
   - 移除 `enabled` 字段后，启用/禁用逻辑需要通过删除规则或其他机制实现

3. **for_times 计算：**
   - 不再存储 `for_times`
   - 新的评估逻辑直接查询 `for_duration` 窗口内的所有数据点
   - 不再需要计数，而是检查所有点是否都满足条件

## 待处理事项

### 必须完成（才能正常工作）：

1. **AlertManager 适配** - 需要更新以下内容：
   - 移除对 `rule.eval_every` 的引用
   - 实现统一的评估频率配置
   - 移除对 `rule.enabled` 的引用
   - 移除对 `rule.for_times` 的引用

2. **旧代码清理：**
   - AlertServices.cpp 中的旧 `BasicAlertEvaluator` 实现已过时
   - 建议完全使用 AlertEvaluatorV2.cpp 中的新实现
   - 或者删除 AlertServices.cpp 中的旧评估器代码

### 可选改进：

1. 添加规则启用/禁用的替代机制
2. 提供数据迁移脚本（从旧格式转换到新格式）
3. 更新 API 文档
4. 编写单元测试

## 文件清单

### 已修改的文件：
1. `/Users/panjinxue/编程/yw2/include/yw/alert_model.h` - Rule结构定义
2. `/Users/panjinxue/编程/yw2/docs/alert_rule_setup_v2.sql` - 数据库表结构
3. `/Users/panjinxue/编程/yw2/src/alert/DatabaseRuleRepository.cpp` - 数据库操作
4. `/Users/panjinxue/编程/yw2/src/alert/AlertEvaluatorV2.cpp` - 评估器context

### 已创建的文件：
1. `/Users/panjinxue/编程/yw2/src/alert/AlertEvaluatorV2.cpp` - 新的评估器实现
2. `/Users/panjinxue/编程/yw2/docs/alert_refactor_plan.md` - 重构计划文档
3. `/Users/panjinxue/编程/yw2/docs/alert_rule_simplified.md` - 本文档

## 使用示例

参考 `alert_rule_setup_v2.sql` 中的示例规则，或查看 `alert_refactor_plan.md` 第九节的详细示例。
