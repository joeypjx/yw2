# Alert 模块告警引擎逻辑说明

## 概述

告警引擎（AlertEngine）是 Alert 模块的核心组件，负责定期评估告警规则、管理告警状态生命周期，并将告警持久化到数据库。

## 架构设计

### 模块分层

```
┌─────────────────────────────────────┐
│   AlertModule (接口适配层)          │
│   - 实现 IAlertModule 接口           │
│   - 提供 JSON 序列化/反序列化        │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   AlertEngine (应用层)               │
│   - 定期评估告警规则                 │
│   - 管理告警状态转换                 │
│   - 处理告警持久化                   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   AlertRuleEvaluator (领域层)       │
│   - 将告警规则转换为 SQL 查询        │
│   - 执行查询并转换为告警对象         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   Repository (基础设施层)            │
│   - AlertRuleRepository              │
│   - AlertRepository                  │
│   - DatabaseQueryInterface          │
└─────────────────────────────────────┘
```

## 核心流程

### 1. 初始化流程

```
AlertEngine::start()
  ├─> initialize()
  │     └─> 从数据库加载所有启用的告警规则到内存
  └─> 启动工作线程 workerLoop()
```

### 2. 主循环流程（workerLoop）

```
while (!shouldStop_) {
  1. performEvaluation()        // 评估硬件资源告警规则
  2. performEvaluationForAlive() // 检查节点心跳超时
  3. sleep(intervalSeconds)      // 等待下次评估（默认5秒）
}
```

### 3. 告警规则评估流程（performEvaluation）

```
performEvaluation()
  ├─> evaluateAllRules()
  │     ├─> 遍历所有启用的告警规则
  │     ├─> 对每个规则调用 AlertRuleEvaluator::evaluateRule()
  │     │     ├─> convertRuleToSQL()      // 将规则转换为 SQL
  │     │     ├─> executeQuery()          // 执行 SQL 查询
  │     │     └─> convertQueryResultToAlerts()  // 转换为告警对象
  │     └─> 返回所有生成的告警
  │
  ├─> processAlertStatusUpdates()
  │     ├─> 获取数据库中当前的 firing 和 pending 告警指纹
  │     ├─> 比较当前生成的告警与数据库中的告警
  │     ├─> 处理状态转换：
  │     │     ├─> Firing → Resolved（如果不再满足条件）
  │     │     └─> Pending → 删除（如果不再满足条件）
  │     └─> 检查节点是否有新数据（避免节点离线时误判）
  │
  └─> updateAlertsToDatabase()
        ├─> 对于新告警：创建到数据库
        ├─> 对于已存在的 Pending 告警：
        │     ├─> 检查是否应该转为 Firing（根据 for 字段）
        │     └─> 如果满足持续时间，转为 Firing 并推送
        └─> 对于已存在的 Firing 告警：更新更新时间
```

### 4. 节点心跳检查流程（performEvaluationForAlive）

```
performEvaluationForAlive()
  ├─> 查询所有节点的最新心跳时间
  ├─> 计算距离现在的时间差（秒）
  ├─> 对于每个节点：
  │     ├─> 如果超过阈值（默认5秒）：
  │     │     ├─> 检查是否已存在 firing 告警
  │     │     ├─> 如果不存在，创建新的 firing 告警
  │     │     └─> 调用推送回调
  │     └─> 如果未超过阈值：
  │           └─> 解决已存在的 firing 告警（如果有）
  └─> 返回处理的节点数量
```

## 告警状态生命周期

### 状态定义

- **Pending（等待中）**：告警条件已满足，但未达到持续时间阈值
- **Firing（触发中）**：告警条件满足且持续时间达到阈值，或立即触发
- **Resolved（已解决）**：告警条件不再满足，告警已解决

### 状态转换流程

```
条件满足
  │
  ├─> for 字段为空或为 0
  │     └─> 直接设为 Firing
  │
  └─> for 字段有值（如 "5m"）
        └─> 设为 Pending
              └─> 等待持续时间
                    └─> 转为 Firing
                          │
                          ├─> 条件持续满足 → 保持 Firing（更新 updated_at）
                          │
                          └─> 条件不再满足 → 转为 Resolved
```

### 状态转换判断逻辑

1. **Pending → Firing**：
   - 在 `updateAlertsToDatabase()` 中检查
   - 调用 `shouldTransitionToFiring()` 判断
   - 计算告警创建时间到现在的持续时间
   - 如果持续时间 >= for 字段设置的时间，则转为 Firing

2. **Firing → Resolved**：
   - 在 `processAlertStatusUpdates()` 中检查
   - 如果告警不再满足条件，且节点有新数据，则转为 Resolved
   - 调用 `alertRepo_->resolveFiringAlertsByFingerprint()`

3. **Pending → 删除**：
   - 如果告警不再满足条件，且节点有新数据，则直接删除

## 告警规则评估（AlertRuleEvaluator）

### SQL 查询生成

告警规则表达式转换为 SQL 查询：

```sql
SELECT DISTINCT ON (host_ip, [tag_columns])
  host_ip, [tag_columns], [metric]
FROM [table_name]
WHERE time >= NOW() - INTERVAL '10 seconds'
  AND [tag_conditions]
  AND [metric_conditions]
ORDER BY host_ip, [tag_columns], time DESC
```

### 支持的资源类型（stable）

- `cpu` → `resource_cpu`
- `memory` → `resource_memory`
- `disk` → `resource_disk`（标签：device, mount_point）
- `network` → `resource_network`（标签：interface）
- `gpu` → `resource_gpu`（标签：gpu_index）
- `alive` → `resource_alive`

### 条件判断

支持的操作符：
- `>`、`<`、`>=`、`<=`、`=`（或 `==`）、`!=`（或 `<>`）

所有条件必须同时满足（AND 逻辑）。

## 告警指纹（Fingerprint）

告警指纹用于去重，相同指纹的告警会被视为同一个告警。

生成规则：
```
fingerprint = alert_name + "|" + tag1=value1 + "|" + tag2=value2 + ...
```

例如：
```
节点CPU使用率过高|host_ip=192.168.1.100|interface=eth0
```

## 特殊告警类型

### 1. 节点心跳超时告警

- **触发条件**：节点最新心跳时间距离现在超过阈值（默认5秒）
- **状态**：直接设为 Firing（不经过 Pending）
- **指纹**：`节点心跳超时|host_ip=<ip>`
- **处理**：在 `performEvaluationForAlive()` 中独立处理

### 2. 业务组件状态异常告警

- **触发方式**：通过 `createAlertFromComponent()` 手动创建
- **状态**：直接设为 Firing
- **指纹**：`业务组件状态异常|host_ip=<ip>|instance_id=<id>|uuid=<uuid>|index=<index>`

### 3. 节点板卡类型变化告警

- **触发方式**：通过 `createBoardTypeChangeAlert()` 手动创建
- **状态**：直接设为 Firing，但 `ends_at` 等于 `created_at`（已解决状态）
- **指纹**：`节点板卡类型变化|box_id=<id>|slot_id=<id>`

## 数据持久化

### 告警规则

- 存储在 `alert_rules` 表
- 启动时加载所有启用的规则到内存
- 规则变更时同步更新内存和数据库

### 告警事件

- 存储在 `alert` 表
- 通过 `Alert::updateInDatabase()` 方法保存
- 支持去重：相同 fingerprint 的告警不会重复创建

## 推送机制

### 推送时机

1. **新创建的 Firing 告警**：在 `updateAlertsToDatabase()` 中
2. **Pending → Firing 转换**：在状态转换时
3. **节点心跳超时告警**：创建时立即推送
4. **业务组件告警**：创建时立即推送

### 推送回调

通过 `setPushCallback()` 设置回调函数，告警引擎会在适当时机调用：

```cpp
void setPushCallback(std::function<void(const Alert&)> callback);
```

## 性能优化

### 1. 内存缓存

- 告警规则加载到内存，避免每次评估都查询数据库
- 规则变更时同步更新内存

### 2. 数据时效性

- 只查询最近10秒的数据（`WHERE time >= NOW() - INTERVAL '10 seconds'`）
- 减少查询数据量，提高评估速度

### 3. 节点数据检查

- 在解决告警前检查节点是否有新数据
- 避免节点离线时误判告警已解决

## 配置参数

- **评估间隔**：默认 5 秒（可通过 `start(intervalSeconds)` 设置）
- **心跳超时阈值**：默认 5 秒（从配置文件 `alert.heartbeat_timeout_seconds` 读取）
- **数据查询窗口**：10 秒（硬编码在 `convertRuleToSQL()` 中）

## 错误处理

- 评估过程中的异常会被捕获并记录日志
- 单个规则评估失败不影响其他规则
- 数据库操作失败会记录错误但不中断主循环

## 统计信息

告警引擎维护以下统计信息：

- `totalEvaluations_`：总评估次数
- `totalAlertsGenerated_`：生成的告警总数
- `lastEvaluationTime_`：最后一次评估时间
- `startTime_`：启动时间

