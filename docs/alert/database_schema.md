# Alert 模块数据库表结构文档

本文档描述了 Alert 模块使用的所有数据库表结构。Alert 模块的表为普通的 PostgreSQL 表，不是 TimescaleDB 时序表。

## 1. alert_rules - 告警规则表

用于存储告警规则的配置信息，定义何时触发告警以及告警的详细信息。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| id | VARCHAR(100) | PRIMARY KEY | 系统生成的唯一标识符 |
| alert_name | VARCHAR(200) | NOT NULL UNIQUE | 告警规则标识，用户自定义 |
| expression | JSONB | NOT NULL | 告警表达式（JSON格式），包含 stable、metric、conditions、tags 等 |
| for_duration | VARCHAR(20) | NOT NULL | 满足持续时间才产生告警，支持 s/m/h 单位（如 "5s", "30s", "1h"） |
| severity | VARCHAR(50) | NOT NULL | 告警等级，用户自定义（如 "严重", "一般", "警告"） |
| summary | TEXT | NOT NULL | 告警摘要，一小段话，用户自定义 |
| description | TEXT | NOT NULL DEFAULT '' | 告警详情，一大段话，用户自定义，支持占位符 {{}} |
| alert_type | VARCHAR(100) | NOT NULL | 告警类型，用户自定义（如 "硬件资源", "系统告警"） |
| enabled | BOOLEAN | NOT NULL DEFAULT TRUE | 是否启用，默认为 true |
| created_at | TIMESTAMP | NOT NULL DEFAULT NOW() | 创建时间，系统自动生成 |
| updated_at | TIMESTAMP | NOT NULL DEFAULT NOW() | 更新时间，系统自动生成，通过触发器自动更新 |

**说明**：
- `expression` 字段为 JSONB 类型，存储告警表达式的完整配置
  - `stable`: 数据源类型（如 "cpu", "disk", "memory", "alive"）
  - `metric`: 指标名称（如 "usage_percent", "alive"）
  - `conditions`: 条件数组，包含 operator（如 ">", "==", "<"）和 threshold（阈值）
  - `tags`: 标签数组，用于过滤特定资源（如 `{"mount_point": "/data"}`）
- `for_duration` 表示告警条件需要持续多长时间才会触发告警
- `description` 支持占位符，如 `{{host_ip}}`、`{{mount_point}}` 等，会在生成告警时替换为实际值
- `updated_at` 字段通过触发器自动更新，每次更新记录时自动设置为当前时间

## 2. alert - 告警事件表

用于存储系统中实际产生的告警事件，记录告警的完整生命周期。

| 字段名 | 数据类型 | 约束 | 说明 |
|--------|----------|------|------|
| id | VARCHAR(200) | PRIMARY KEY | 告警ID，系统生成的唯一标识符 |
| fingerprint | VARCHAR(500) | NOT NULL | 告警指纹，用于去重和识别，格式为 alert_name\|key1=value1\|key2=value2 |
| labels | JSONB | NOT NULL | 告警标签，JSON格式，包含节点IP、指标值、告警规则信息等 |
| annotations | JSONB | NOT NULL | 告警注释，JSON格式，包含 summary 和 description |
| created_at | TIMESTAMP | NOT NULL DEFAULT NOW() | 第一次匹配时间，可能还未满足持续时间条件 |
| starts_at | TIMESTAMP | | 第一次正式触发告警时间，满足持续时间条件 |
| updated_at | TIMESTAMP | NOT NULL DEFAULT NOW() | 触发中持续匹配的更新时间，每次更新，通过触发器自动更新 |
| ends_at | TIMESTAMP | | 告警已解决的时间 |
| status | VARCHAR(20) | NOT NULL DEFAULT 'pending' | 告警状态：pending/firing/resolved |
| alert_rule_id | VARCHAR(100) | | 关联的告警规则ID |

**约束**：
- `status` 字段必须为以下值之一：'pending', 'firing', 'resolved'

**说明**：
- `fingerprint` 用于告警去重，相同指纹的告警会被合并为同一条记录
- `labels` 字段包含告警的元数据，常见字段包括：
  - `alert_name`: 告警名称
  - `alert_type`: 告警类型
  - `host_ip`: 节点IP地址
  - `metric`: 指标名称
  - `severity`: 告警等级
  - `stable`: 数据源类型
  - `value`: 指标值
  - 其他标签（如 `mount_point`、`interface` 等）
- `annotations` 字段包含告警的可读信息：
  - `summary`: 告警摘要
  - `description`: 告警详情（已替换占位符）
- `status` 字段表示告警的生命周期状态：
  - `pending`: 已匹配告警规则，但还未满足持续时间条件
  - `firing`: 已满足持续时间条件，正式触发告警
  - `resolved`: 告警条件不再满足，告警已解决
- `updated_at` 字段通过触发器自动更新，每次更新记录时自动设置为当前时间

## 表关系说明

- `alert` 表通过 `alert_rule_id` 字段关联到 `alert_rules` 表
- `alert` 表的 `labels` 字段中的 `host_ip` 可以与 monitor 模块的资源表通过 `host_ip` 进行关联
- `alert_rules` 表的 `expression` 字段中的 `stable` 和 `metric` 用于匹配 monitor 模块的资源数据

## 数据流程说明

### 告警规则创建流程
1. 用户在系统中创建告警规则，数据保存到 `alert_rules` 表
2. 告警规则包含表达式、持续时间、严重程度等信息
3. 规则可以启用或禁用（`enabled` 字段）

### 告警触发流程
1. 告警引擎定期检查 monitor 模块的资源数据
2. 根据 `alert_rules` 表中的规则表达式匹配数据
3. 如果匹配成功，创建或更新 `alert` 表中的记录
4. 告警状态从 `pending` 变为 `firing`（满足持续时间后）
5. 告警条件不再满足时，状态变为 `resolved`

## 注意事项

1. `alert_rules` 和 `alert` 表都是普通的 PostgreSQL 表，不是 TimescaleDB 时序表
2. 两个表都创建了相应的索引以优化查询性能
3. 两个表都使用触发器自动更新 `updated_at` 字段
4. `alert_rules.expression` 和 `alert.labels`、`alert.annotations` 字段使用 JSONB 类型，支持高效的 JSON 查询
5. `alert.status` 字段有 CHECK 约束，确保只能使用预定义的状态值
6. `alert.fingerprint` 字段用于告警去重，相同指纹的告警会被合并
7. `alert_rules.alert_name` 字段有 UNIQUE 约束，确保告警规则名称唯一

