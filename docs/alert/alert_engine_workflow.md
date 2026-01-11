# 告警引擎工作原理文档

本文档详细说明告警引擎（AlertEngine）的架构设计、工作流程和核心机制。

## 目录

1. [概述](#概述)
2. [架构设计](#架构设计)
3. [核心组件](#核心组件)
4. [工作流程](#工作流程)
5. [状态管理](#状态管理)
6. [规则评估机制](#规则评估机制)
7. [告警去重机制](#告警去重机制)
8. [性能优化](#性能优化)

## 概述

告警引擎是告警系统的核心组件，负责定期评估告警规则，监控系统资源状态，并在满足告警条件时生成和管理告警事件。引擎采用事件驱动架构，通过数据库作为数据源，实现高效的规则评估和状态管理。

### 主要功能

- **规则评估**：定期执行所有启用的告警规则，评估条件是否满足
- **告警生成**：当规则条件满足时，创建或更新告警事件
- **状态管理**：管理告警事件的状态转换（Pending → Firing → Resolved）
- **持续时间检查**：检查告警是否持续满足条件达到指定的持续时间（`for`字段）
- **回调通知**：当告警状态变化时，调用推送回调函数通知外部系统
- **数据持久化**：将告警规则和告警事件持久化到数据库

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      AlertModuleAdapter                      │
│  (实现 IAlertModule 接口，统一封装告警系统各个组件)          │
└─────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│ AlertEngine  │   │AlertRule     │   │AlertQuery    │
│  (评估引擎)   │   │Service       │   │Service       │
│              │   │ (规则管理)    │   │ (查询服务)    │
└──────────────┘   └──────────────┘   └──────────────┘
        │                   │                   │
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│AlertRule     │   │AlertRule     │   │AlertEvent    │
│Evaluator     │   │Repository    │   │Repository    │
│ (规则评估器)  │   │ (规则仓库)    │   │ (事件仓库)    │
└──────────────┘   └──────────────┘   └──────────────┘
        │                   │                   │
        └───────────────────┼───────────────────┘
                            │
                            ▼
                    ┌──────────────┐
                    │  PostgreSQL  │
                    │   Database   │
                    └──────────────┘
```

### 分层设计

告警系统采用分层架构，分为以下层次：

1. **接口层（Interface Layer）**
   - `IAlertModule`：告警模块公共接口
   - `AlertModuleAdapter`：适配器实现，统一封装各个组件

2. **应用层（Application Layer）**
   - `AlertEngine`：告警引擎，负责定期评估和状态管理
   - `AlertRuleService`：告警规则服务，管理规则的增删改查
   - `AlertQueryService`：告警查询服务，提供查询功能
   - `AlertCreationFactory`：告警创建工厂，创建特定类型的告警

3. **领域层（Domain Layer）**
   - `AlertRule`：告警规则领域模型
   - `AlertEvent`：告警事件领域模型
   - `AlertRuleEvaluator`：规则评估器，负责SQL转换和评估

4. **基础设施层（Infrastructure Layer）**
   - `AlertRuleRepository`：告警规则仓库，负责数据持久化
   - `AlertEventRepository`：告警事件仓库，负责数据持久化
   - `DatabaseQueryInterface`：数据库查询接口，封装SQL执行

## 核心组件

### 1. AlertEngine（告警引擎）

告警引擎是系统的核心，负责定期评估告警规则并管理告警状态。

**主要职责**：
- 启动和停止评估循环
- 定期执行所有启用的告警规则
- 处理告警状态转换
- 更新告警到数据库
- 触发推送回调

**关键方法**：
- `start(intervalSeconds)`：启动引擎，开始定期评估
- `stop()`：停止引擎
- `performEvaluation()`：执行一次完整的告警评估
- `evaluateAllRules()`：评估所有启用的告警规则
- `processAlertStatusUpdates()`：处理告警状态更新
- `updateAlertsToDatabase()`：将告警更新到数据库

### 2. AlertRuleEvaluator（规则评估器）

规则评估器负责将告警规则转换为SQL查询并执行评估。

**主要职责**：
- 将告警规则转换为PostgreSQL SQL查询
- 执行SQL查询获取数据
- 将查询结果转换为告警事件
- 检查指标值是否满足告警条件

**关键方法**：
- `evaluateRule(rule)`：评估单个告警规则
- `convertRuleToSQL(rule)`：将规则转换为SQL查询
- `convertQueryResultToAlerts(result, rule)`：将查询结果转换为告警事件
- `checkAlertConditions(value, conditions)`：检查条件是否满足

### 3. AlertRule（告警规则）

告警规则定义了告警的触发条件和元数据。

**主要属性**：
- `id`：规则唯一标识符
- `alert_name`：告警名称
- `expression`：告警表达式（包含条件、指标、表名、标签）
- `for`：持续时间（如"5m"，表示持续5分钟后才触发告警）
- `severity`：严重程度
- `summary`：摘要
- `description`：详细描述（支持{{}}占位符）
- `alert_type`：告警类型
- `enabled`：是否启用

### 4. AlertEvent（告警事件）

告警事件表示一个具体的告警实例。

**主要属性**：
- `fingerprint`：告警指纹（唯一标识符）
- `labels`：标签（包含告警的基本信息和指标值）
- `annotations`：注释（包含摘要和描述）
- `status`：状态（Pending/Firing/Resolved）
- `starts_at`：开始时间
- `ends_at`：结束时间
- `created_at`：创建时间
- `updated_at`：更新时间

## 工作流程

### 引擎启动流程

```
1. 创建 AlertEngine 实例
   ↓
2. 调用 start(intervalSeconds)
   ↓
3. 初始化告警规则（从数据库加载）
   ↓
4. 启动工作线程（workerLoop）
   ↓
5. 启动节点存活检查（AlertCreationFactory）
   ↓
6. 引擎进入运行状态
```

### 评估循环流程

告警引擎以固定间隔（默认5秒）执行评估循环：

```
┌─────────────────────────────────────────┐
│   workerLoop() - 工作线程主循环          │
└─────────────────────────────────────────┘
                    │
                    ▼
        ┌───────────────────────┐
        │ performEvaluation()   │
        │ 执行一次完整评估        │
        └───────────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
        ▼                       ▼
┌──────────────┐      ┌──────────────────┐
│evaluateAll   │      │processAlertStatus│
│Rules()       │      │Updates()         │
│评估所有规则   │      │处理状态更新        │
└──────────────┘      └──────────────────┘
        │                       │
        └───────────┬───────────┘
                    │
                    ▼
        ┌───────────────────────┐
        │updateAlertsToDatabase │
        │更新告警到数据库         │
        └───────────────────────┘
                    │
                    ▼
        等待 intervalSeconds 秒
                    │
                    └─── 循环继续
```

### 规则评估流程

单个告警规则的评估流程：

```
┌─────────────────────────────────────────┐
│  AlertRuleEvaluator::evaluateRule()     │
└─────────────────────────────────────────┘
                    │
                    ▼
        1. 检查规则是否启用
                    │
                    ▼
        2. 将规则转换为SQL查询
        convertRuleToSQL(rule)
                    │
                    ▼
        3. 执行SQL查询
        dbInterface_->executeQuery(sql)
                    │
                    ▼
        4. 将查询结果转换为告警事件
        convertQueryResultToAlerts(result, rule)
                    │
                    ├─── 遍历每一行数据
                    │
                    ├─── 检查指标值是否满足条件
                    │
                    ├─── 构建标签（labels）
                    │
                    ├─── 构建注释（annotations）
                    │
                    ├─── 生成指纹（fingerprint）
                    │
                    └─── 创建告警对象（AlertEvent）
                    │
                    ▼
        5. 返回告警事件列表
```

### SQL查询生成

告警规则转换为SQL查询的示例：

**告警规则示例**：
```json
{
  "alert_name": "cpu_high",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">", "threshold": 80}
    ],
    "tags": []
  }
}
```

**生成的SQL查询**：
```sql
SELECT DISTINCT ON (host_ip) 
    host_ip, usage_percent 
FROM resource_cpu 
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND usage_percent > 80::numeric
ORDER BY host_ip, time DESC
```

**带标签过滤的规则示例**：
```json
{
  "expression": {
    "stable": "disk",
    "metric": "usage_percent",
    "conditions": [
      {"operator": ">", "threshold": 90}
    ],
    "tags": [
      {"mount_point": "/data"}
    ]
  }
}
```

**生成的SQL查询**：
```sql
SELECT DISTINCT ON (host_ip, device, mount_point) 
    host_ip, device, mount_point, usage_percent 
FROM resource_disk 
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND (mount_point = '/data')
  AND usage_percent > 90::numeric
ORDER BY host_ip, device, mount_point, time DESC
```

## 状态管理

### 告警状态

告警事件有三种状态：

1. **Pending（待触发）**
   - 刚满足告警条件，但未达到持续时间要求
   - 等待 `for` 字段指定的时间后转为 Firing
   - 如果在此期间条件不再满足，告警会被删除

2. **Firing（触发中）**
   - 持续满足告警条件达到指定时间
   - 表示告警已正式触发
   - 会触发推送回调通知外部系统

3. **Resolved（已解决）**
   - 不再满足告警条件
   - 表示问题已解决
   - 告警结束，`ends_at` 字段被设置

### 状态转换流程

```
条件满足
    │
    ▼
┌─────────┐
│ Pending │  ←─── 初始状态（如果设置了for字段）
└─────────┘
    │
    │ 持续满足条件 + 达到持续时间
    │
    ▼
┌─────────┐
│ Firing  │  ←─── 触发状态
└─────────┘
    │
    │ 条件不再满足
    │
    ▼
┌──────────┐
│ Resolved │  ←─── 解决状态
└──────────┘
```

### 状态转换逻辑

#### Pending → Firing

转换条件：
1. 告警持续处于 Pending 状态
2. 持续满足告警条件
3. 持续时间达到 `for` 字段指定的时间

实现逻辑（`shouldTransitionToFiring`）：
```cpp
1. 获取告警的创建时间（created_at）
2. 计算从创建时间到现在的持续时间
3. 解析规则的 for 字段（如"5m" → 300秒）
4. 如果持续时间 >= for 字段指定的时间，则转为 Firing
```

#### Firing → Resolved

转换条件：
1. 告警处于 Firing 状态
2. 当前评估不再生成相同指纹的告警（条件不再满足）
3. 节点有最近的数据（确保不是数据缺失导致的）

实现逻辑（`processAlertStatusUpdates`）：
```cpp
1. 获取数据库中所有 Firing 状态的告警指纹
2. 获取当前评估生成的所有告警指纹
3. 找出不再满足条件的 Firing 告警
4. 检查节点是否有最近的数据（hasNodeRecentData）
5. 如果有新数据但不满足条件，标记为 Resolved
```

#### Pending 删除

删除条件：
1. 告警处于 Pending 状态
2. 当前评估不再生成相同指纹的告警（条件不再满足）
3. 节点有最近的数据（确保不是数据缺失导致的）

实现逻辑：
```cpp
1. 获取数据库中所有 Pending 状态的告警指纹
2. 获取当前评估生成的所有告警指纹
3. 找出不再满足条件的 Pending 告警
4. 检查节点是否有最近的数据
5. 如果有新数据但不满足条件，直接删除
```

### 节点数据检查

为了避免因数据缺失导致的误判，引擎会检查节点是否有最近的数据：

**检查逻辑（`hasNodeRecentData`）**：
```cpp
1. 从告警的 labels 中获取 host_ip 和 stable
2. 根据 stable 确定对应的数据库表名
3. 查询该节点在最近10秒内是否有新数据
4. 如果有标签列（如 disk 的 device、mount_point），添加标签过滤
5. 返回是否有新数据
```

**示例查询**：
```sql
SELECT COUNT(*) as count 
FROM resource_cpu 
WHERE host_ip = '192.168.2.101'::inet
  AND time >= NOW() - INTERVAL '10 seconds'
```

## 规则评估机制

### 规则评估步骤

1. **规则过滤**
   - 只评估启用的规则（`enabled = true`）
   - 从 `AlertRuleService` 获取所有启用的规则

2. **SQL转换**
   - 将告警规则表达式转换为PostgreSQL SQL查询
   - 使用 `DISTINCT ON` 获取每个节点（和标签组合）的最新数据
   - 查询最近10秒的数据（`time >= NOW() - INTERVAL '10 seconds'`）

3. **条件构建**
   - **标签条件**：将 `expression.tags` 转换为 SQL WHERE 条件
     - 多个标签组之间用 `OR` 连接
     - 标签组内的标签用 `AND` 连接
   - **指标条件**：将 `expression.conditions` 转换为 SQL WHERE 条件
     - 多个条件之间用 `AND` 连接
     - 支持操作符：`>`, `<`, `>=`, `<=`, `=`, `==`, `!=`, `<>`

4. **查询执行**
   - 执行SQL查询，获取满足条件的数据行

5. **结果转换**
   - 遍历查询结果的每一行
   - 检查指标值是否满足告警条件（双重检查）
   - 构建标签（labels）和注释（annotations）
   - 生成指纹（fingerprint）
   - 创建告警对象

### 表名映射

告警规则中的 `stable` 字段映射到数据库表名：

| stable | 数据库表名 |
|--------|-----------|
| cpu | resource_cpu |
| memory | resource_memory |
| disk | resource_disk |
| network | resource_network |
| gpu | resource_gpu |
| alive | resource_alive |

### 标签列映射

不同资源类型有不同的标签列，用于告警分组：

| stable | 标签列 |
|--------|--------|
| cpu | 无 |
| memory | 无 |
| disk | device, mount_point |
| network | interface |
| gpu | gpu_index |
| alive | 无 |

### 条件检查

告警规则支持多个条件，所有条件必须同时满足：

**条件示例**：
```json
{
  "conditions": [
    {"operator": ">", "threshold": 80},
    {"operator": "<", "threshold": 100}
  ]
}
```

**生成的SQL条件**：
```sql
usage_percent > 80::numeric AND usage_percent < 100::numeric
```

**代码检查**：
```cpp
// 在 convertQueryResultToAlerts 中，对每行数据再次检查条件
double metricValue = row.getDoubleValue(rule.getExpression().metric);
bool conditionMet = checkAlertConditions(metricValue, rule.getExpression().conditions);
if (!conditionMet) {
    continue; // 跳过不满足条件的行
}
```

## 告警去重机制

### 指纹（Fingerprint）生成

告警指纹用于唯一标识一个告警实例，相同指纹的告警会被去重。

**指纹组成**：
```
fingerprint = alert_name + "|" + tag1=value1 + "|" + tag2=value2 + ...
```

**指纹标签**：
- `host_ip`：节点IP地址（必需）
- `expression.tags` 中的所有标签（可选）

**示例**：
```
规则名称: cpu_high
标签: {host_ip: "192.168.2.101"}

指纹: "cpu_high|host_ip=192.168.2.101/32"
```

**带标签的示例**：
```
规则名称: disk_high
标签: {host_ip: "192.168.2.101", device: "/dev/sda5", mount_point: "/data"}

指纹: "disk_high|device=/dev/sda5|host_ip=192.168.2.101/32|mount_point=/data"
```

### 去重逻辑

在 `updateAlertsToDatabase` 中实现去重：

1. **检查现有告警**
   ```cpp
   auto existingAlert = alertRepo_->getAlertByFingerprint(alertCopy.getFingerprint());
   ```

2. **Pending 告警处理**
   - 如果数据库中已有 Pending 告警，且新告警也是 Pending：
     - 检查是否需要转为 Firing（`shouldTransitionToFiring`）
     - 如果需要，更新状态并触发回调
     - 如果不需要，保持 Pending 状态，不更新数据库
   - 如果数据库中已有 Pending 告警，但新告警是 Firing：
     - 直接转为 Firing，更新数据库并触发回调

3. **Firing 告警处理**
   - 如果数据库中已有 Firing 告警，且新告警也是 Firing：
     - 只更新 `updated_at` 时间戳，不触发回调
     - 表示告警仍在持续

4. **新告警创建**
   - 如果不存在现有告警，创建新告警
   - 只有 Firing 状态的告警才触发推送回调

## 性能优化

### 1. 查询优化

- **时间窗口限制**：只查询最近10秒的数据，避免扫描整个表
- **DISTINCT ON**：使用 PostgreSQL 的 `DISTINCT ON` 获取每个节点的最新数据
- **索引利用**：依赖数据库表的索引（如 `host_ip`, `time`）

### 2. 状态过滤

- **按类型过滤**：在查询 Firing 和 Pending 告警时，只查询"硬件状态"类型的告警
- **减少数据传输**：只获取告警指纹，不获取完整告警对象

### 3. 批量处理

- **批量更新**：将多个告警的更新操作合并处理
- **事务管理**：使用数据库事务确保数据一致性

### 4. 异步处理

- **独立线程**：告警评估在独立线程中运行，不阻塞主线程
- **可中断等待**：使用分段 sleep，支持快速停止

### 5. 缓存机制

- **规则缓存**：`AlertRuleService` 缓存启用的规则，避免频繁查询数据库
- **指纹集合**：使用 `unordered_set` 存储指纹，快速查找

## 总结

告警引擎通过定期评估、状态管理和去重机制，实现了高效的告警监控系统。核心特点包括：

1. **事件驱动**：基于数据库数据变化触发告警
2. **状态管理**：支持 Pending → Firing → Resolved 的状态转换
3. **去重机制**：通过指纹避免重复告警
4. **性能优化**：通过查询优化和缓存机制提高性能
5. **可扩展性**：分层架构便于扩展和维护

通过以上机制，告警引擎能够可靠地监控系统状态，及时发现问题并通知相关人员。
