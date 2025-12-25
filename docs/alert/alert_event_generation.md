# 告警事件生成流程文档

本文档详细说明告警引擎如何根据 SQL 查询结果生成告警事件（AlertEvent）的完整流程。

## 概述

告警事件生成是告警引擎的核心流程之一，它将数据库查询结果转换为结构化的告警事件对象，并处理状态转换、去重、持久化等逻辑。

## 整体流程

```
SQL查询结果 (QueryResult)
    ↓
遍历每一行数据
    ↓
条件检查 (checkAlertConditions)
    ↓
构建标签 (Labels)
    ↓
构建注释 (Annotations) - 占位符替换
    ↓
生成指纹 (Fingerprint)
    ↓
创建告警对象 (AlertEvent)
    ↓
设置初始状态 (Pending/Firing)
    ↓
返回告警列表
    ↓
AlertEngine 处理
    ├─ 状态更新 (processAlertStatusUpdates)
    └─ 数据库持久化 (updateAlertsToDatabase)
```

## 详细步骤

### 步骤 1: SQL 查询执行

**位置**: `AlertRuleEvaluator::evaluateRule()`

```cpp
// 1. 将告警规则转换为SQL查询
std::string sql = convertRuleToSQL(rule);

// 2. 执行查询
QueryResult result = dbInterface_->executeQuery(sql);

// 3. 将查询结果转换为告警对象
return convertQueryResultToAlerts(result, rule);
```

**说明**:
- SQL 查询返回 `QueryResult` 对象，包含多行数据
- 每行数据代表一个可能触发告警的资源实例（如某个节点的 CPU、某个磁盘分区等）

### 步骤 2: 遍历查询结果

**位置**: `AlertRuleEvaluator::convertQueryResultToAlerts()`

```cpp
std::vector<AlertEvent> alerts;

for (const auto& row : result.rows) {
    // 处理每一行数据
    // ...
}
```

**说明**:
- 对查询结果的每一行进行处理
- 每行可能生成一个告警事件（如果满足条件）

### 步骤 3: 条件检查（双重验证）

**位置**: `AlertRuleEvaluator::checkAlertConditions()`

```cpp
// 从查询结果中获取指标值
double metricValue = row.getDoubleValue(rule.getExpression().metric);

// 检查是否满足告警条件（应用层验证）
bool conditionMet = checkAlertConditions(metricValue, rule.getExpression().conditions);

if (!conditionMet) {
    continue; // 跳过不满足条件的行
}
```

**条件检查逻辑**:

```cpp
bool AlertRuleEvaluator::checkAlertConditions(double value, 
                                             const std::vector<AlertCondition>& conditions) {
    for (const auto& condition : conditions) {
        bool conditionMet = false;
        
        if (condition.operator_ == ">") {
            conditionMet = (value > condition.threshold);
        } else if (condition.operator_ == "<") {
            conditionMet = (value < condition.threshold);
        } else if (condition.operator_ == ">=") {
            conditionMet = (value >= condition.threshold);
        } else if (condition.operator_ == "<=") {
            conditionMet = (value <= condition.threshold);
        } else if (condition.operator_ == "=" || condition.operator_ == "==") {
            conditionMet = (value == condition.threshold);
        } else if (condition.operator_ == "!=" || condition.operator_ == "<>") {
            conditionMet = (value != condition.threshold);
        }
        
        if (!conditionMet) {
            return false; // 任何一个条件不满足就返回false
        }
    }
    
    return true; // 所有条件都满足
}
```

**说明**:
- **双重验证**: SQL 查询中已经包含了条件过滤，但应用层再次验证以确保准确性
- **AND 逻辑**: 所有条件必须同时满足（AND 关系）
- **操作符支持**: `>`, `<`, `>=`, `<=`, `==`, `=`, `!=`, `<>`

### 步骤 4: 构建标签（Labels）

**位置**: `AlertRuleEvaluator::convertQueryResultToAlerts()`

标签是告警事件的元数据，用于标识和分类告警。

```cpp
std::unordered_map<std::string, std::string> labels;

// 1. 基础标签（来自告警规则）
labels["alert_name"] = rule.getAlertName();
labels["alert_type"] = rule.getAlertType();
labels["severity"] = rule.getSeverity();
labels["stable"] = rule.getExpression().stable;
labels["metric"] = rule.getExpression().metric;

// 2. 从查询结果获取主机IP
std::string hostIp = row.getValue("host_ip");
labels["host_ip"] = hostIp;
labels["value"] = std::to_string(metricValue);

// 3. 通过 IPAddressUtils 解析 box_id 和 slot_id
auto boxSlotInfo = yw::utils::IPAddressUtils::parseHostIP(hostIp);
if (boxSlotInfo.has_value()) {
    labels["box_id"] = std::to_string(boxSlotInfo->box_id);
    labels["slot_id"] = std::to_string(boxSlotInfo->slot_id);
}

// 4. 添加标签列的值（根据 stable 类型）
auto tagColumns = getTagColumns(rule.getExpression().stable);
for (const auto& column : tagColumns) {
    std::string value = row.getValue(column);
    if (!value.empty()) {
        labels[column] = value;
    }
}
```

**标签列映射**:

| stable | 标签列 |
|--------|--------|
| `cpu` | 无 |
| `memory` | 无 |
| `disk` | `device`, `mount_point` |
| `network` | `interface` |
| `gpu` | `gpu_index` |
| `alive` | 无 |

**示例标签**:
```json
{
  "alert_name": "CPU使用率过高",
  "alert_type": "硬件状态",
  "severity": "严重",
  "stable": "cpu",
  "metric": "usage_percent",
  "host_ip": "192.168.1.100",
  "box_id": "1",
  "slot_id": "2",
  "value": "85.5"
}
```

### 步骤 5: 构建注释（Annotations）

**位置**: `AlertRuleEvaluator::convertQueryResultToAlerts()`

注释是告警通知的内容，支持占位符替换。

```cpp
std::unordered_map<std::string, std::string> annotations;
annotations["summary"] = fillPlaceholders(rule.getSummary(), labels);
annotations["description"] = fillPlaceholders(rule.getDescription(), labels);
```

**占位符替换逻辑**:

```cpp
std::string AlertRuleEvaluator::fillPlaceholders(
    const std::string& templateStr, 
    const std::unordered_map<std::string, std::string>& labels) {
    
    std::string result = templateStr;
    
    // 替换 {{key}} 格式的占位符
    std::regex placeholderRegex(R"(\{\{([^}]+)\}\})");
    std::smatch match;
    
    while (std::regex_search(result, match, placeholderRegex)) {
        std::string key = match[1].str();
        auto it = labels.find(key);
        std::string value = (it != labels.end()) ? it->second : "";
        result = std::regex_replace(result, std::regex("\\{\\{" + key + "\\}\\}"), value);
    }
    
    return result;
}
```

**示例**:
- 模板: `"节点 {{host_ip}} 的 CPU 使用率为 {{value}}%"`
- 替换后: `"节点 192.168.1.100 的 CPU 使用率为 85.5%"`

### 步骤 6: 生成指纹（Fingerprint）

**位置**: `AlertRuleEvaluator::convertQueryResultToAlerts()`

指纹用于唯一标识相同类型的告警，用于去重和状态管理。

```cpp
// 生成指纹
std::unordered_map<std::string, std::string> fingerprintTags;
fingerprintTags["host_ip"] = labels["host_ip"];

// 添加规则表达式中的标签
for (const auto& tag : rule.getExpression().tags) {
    for (const auto& tagPair : tag) {
        fingerprintTags[tagPair.first] = tagPair.second;
    }
}

std::string fingerprint = AlertEvent::generateFingerprint(rule.getAlertName(), fingerprintTags);
```

**指纹生成逻辑**:

```cpp
std::string AlertEvent::generateFingerprint(
    const std::string& alertName, 
    const std::unordered_map<std::string, std::string>& tags) {
    
    std::ostringstream oss;
    oss << alertName;
    
    // 按key排序以确保一致性
    std::vector<std::pair<std::string, std::string>> sortedTags(tags.begin(), tags.end());
    std::sort(sortedTags.begin(), sortedTags.end());
    
    for (const auto& tag : sortedTags) {
        oss << "|" << tag.first << "=" << tag.second;
    }
    
    return oss.str();
}
```

**指纹格式**: `alert_name|key1=value1|key2=value2`

**示例**:
- 告警名称: `"CPU使用率过高"`
- 标签: `{"host_ip": "192.168.1.100", "mount_point": "/data"}`
- 指纹: `"CPU使用率过高|host_ip=192.168.1.100|mount_point=/data"`

**说明**:
- 相同指纹的告警会被视为同一个告警实例
- 指纹用于去重和状态管理（Pending → Firing → Resolved）

### 步骤 7: 创建告警对象

**位置**: `AlertRuleEvaluator::convertQueryResultToAlerts()`

```cpp
// 创建告警对象
AlertEvent alert(fingerprint, labels, annotations);
```

**AlertEvent 构造函数**:

```cpp
AlertEvent::AlertEvent(
    const std::string& fingerprint, 
    const std::unordered_map<std::string, std::string>& labels,
    const std::unordered_map<std::string, std::string>& annotations)
    : fingerprint_(fingerprint), labels_(labels), annotations_(annotations) {
    
    // 自动生成系统字段
    generateId();        // 生成唯一ID: alert_YYYYMMDD_HHMMSS_毫秒_随机数
    setCreatedNow();     // 设置创建时间
    setUpdatedNow();     // 设置更新时间
    status_ = AlertStatus::Pending;  // 初始状态为Pending
}
```

**系统字段**:
- `id_`: 告警唯一ID（格式: `alert_YYYYMMDD_HHMMSS_毫秒_随机数`）
- `created_at_`: 创建时间（ISO 8601 格式）
- `updated_at_`: 更新时间
- `status_`: 告警状态（初始为 `Pending`）

### 步骤 8: 设置初始状态

**位置**: `AlertRuleEvaluator::convertQueryResultToAlerts()`

根据告警规则的 `for` 字段决定初始状态：

```cpp
// 检查告警规则的for字段
std::string forDuration = rule.getFor();

if (forDuration.empty() || forDuration == "0s" || forDuration == "0m" || forDuration == "0h") {
    // 如果没有设置for字段或为0，直接设为Firing状态
    alert.transitionToFiring();
} else {
    // 如果设置了for字段，设为Pending状态，等待持续时间检查
    alert.transitionToPending();
}
```

**状态说明**:

| 状态 | 说明 | 触发条件 |
|------|------|----------|
| `Pending` | 待确认 | `for` 字段不为空且不为 0 |
| `Firing` | 已触发 | `for` 字段为空或为 0，或 Pending 状态持续满足 `for` 时长 |

**状态转换方法**:

```cpp
void AlertEvent::transitionToPending() {
    status_ = AlertStatus::Pending;
    setUpdatedNow();
    starts_at_.clear();
    ends_at_.clear();
}

void AlertEvent::transitionToFiring() {
    if (status_ == AlertStatus::Pending) {
        status_ = AlertStatus::Firing;
        setStartsNow();  // 设置首次触发时间
        setUpdatedNow();
        ends_at_.clear();
    }
}

void AlertEvent::transitionToResolved() {
    if (status_ == AlertStatus::Firing) {
        status_ = AlertStatus::Resolved;
        setEndsNow();    // 设置解决时间
        setUpdatedNow();
    }
}
```

### 步骤 9: AlertEngine 处理

**位置**: `AlertEngine::performEvaluation()`

生成的告警列表会被 AlertEngine 进一步处理：

```cpp
// 1. 评估所有告警规则，生成当前全量告警
std::vector<AlertEvent> currentAlerts = evaluateAllRules();

// 2. 处理告警状态更新
int processedCount = processAlertStatusUpdates(currentAlerts);

// 3. 更新告警到数据库
int updatedCount = updateAlertsToDatabase(currentAlerts);
```

#### 9.1 状态更新处理

**位置**: `AlertEngine::processAlertStatusUpdates()`

**功能**:
1. **Firing → Resolved**: 如果告警不再满足条件，且节点有新数据，则转为 Resolved
2. **Pending → 删除**: 如果 Pending 告警不再满足条件，且节点有新数据，则删除

**逻辑**:
```cpp
// 获取当前生成的告警指纹
std::unordered_set<std::string> currentAlertFingerprints;
for (const auto& alert : currentAlerts) {
    currentAlertFingerprints.insert(alert.getFingerprint());
}

// 1. 处理firing状态的告警：如果不再满足条件，转为resolved
for (const auto& fingerprint : currentFiringFingerprints) {
    if (currentAlertFingerprints.find(fingerprint) == currentAlertFingerprints.end()) {
        // 告警不再满足条件
        if (hasNodeRecentData(*alert)) {
            // 节点有新数据，转为resolved
            alertRepo_->resolveFiringAlertsByFingerprint(fingerprint);
        }
    }
}

// 2. 处理pending状态的告警：如果不再满足条件，直接删除
for (const auto& fingerprint : currentPendingFingerprints) {
    if (currentAlertFingerprints.find(fingerprint) == currentAlertFingerprints.end()) {
        // 告警不再满足条件
        if (hasNodeRecentData(*alert)) {
            // 节点有新数据，删除pending告警
            alertRepo_->deleteAlertsByFingerprintAndStatus(fingerprint, "pending");
        }
    }
}
```

#### 9.2 数据库持久化

**位置**: `AlertEngine::updateAlertsToDatabase()`

**功能**: 将告警事件保存到数据库，处理去重和状态转换。

**处理逻辑**:

1. **检查是否存在相同指纹的告警**:
   ```cpp
   auto existingAlert = alertRepo_->getAlertByFingerprint(alertCopy.getFingerprint());
   ```

2. **如果已存在告警**:
   - **Pending → Firing**: 如果数据库中已有 Pending 告警，检查是否满足 `for` 时长，满足则转为 Firing
   - **Firing → 更新**: 如果数据库中已有 Firing 告警，更新 `updated_at` 时间戳

3. **如果不存在告警**:
   - 创建新告警（通过 `AlertEvent::updateInDatabase()`）

4. **推送回调**:
   - 状态转换时（Pending → Firing）调用推送回调
   - 新创建的 Firing 告警调用推送回调

**代码示例**:
```cpp
if (existingAlert) {
    if (existingAlert->getStatus() == AlertStatus::Pending) {
        if (alertCopy.getStatus() == AlertStatus::Pending) {
            // 检查是否需要转为firing
            if (shouldTransitionToFiring(*existingAlert)) {
                existingAlert->transitionToFiring();
                existingAlert->updateInDatabase(alertRepo_);
                // 调用推送回调
                if (pushCallback_) {
                    pushCallback_(*existingAlert);
                }
            }
        }
    } else if (existingAlert->getStatus() == AlertStatus::Firing) {
        // 更新现有Firing告警的时间戳
        existingAlert->setUpdatedNow();
        existingAlert->updateInDatabase(alertRepo_);
    }
} else {
    // 创建新告警
    alertCopy.updateInDatabase(alertRepo_);
    if (alertCopy.getStatus() == AlertStatus::Firing && pushCallback_) {
        pushCallback_(alertCopy);
    }
}
```

## 完整示例

### 示例: CPU 使用率告警

**告警规则**:
```json
{
  "alert_name": "CPU使用率过高",
  "expression": {
    "stable": "cpu",
    "metric": "usage_percent",
    "conditions": [{"operator": ">", "threshold": 80.0}],
    "tags": []
  },
  "for": "5s",
  "severity": "严重",
  "summary": "节点 {{host_ip}} CPU使用率过高",
  "description": "节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过阈值80%"
}
```

**SQL 查询结果**:
```
host_ip          | usage_percent
-----------------|---------------
192.168.1.100    | 85.5
192.168.1.101    | 90.2
```

**处理流程**:

1. **第一行数据** (`192.168.1.100`, `85.5`):
   - ✅ 条件检查: `85.5 > 80.0` → 满足
   - 标签: `{"alert_name": "CPU使用率过高", "host_ip": "192.168.1.100", "value": "85.5", ...}`
   - 注释: `{"summary": "节点 192.168.1.100 CPU使用率过高", "description": "节点 192.168.1.100 的CPU使用率为 85.5%，超过阈值80%"}`
   - 指纹: `"CPU使用率过高|host_ip=192.168.1.100"`
   - 状态: `Pending` (因为 `for` 为 "5s")
   - ID: `"alert_20250126_120000_123_5678"`

2. **第二行数据** (`192.168.1.101`, `90.2`):
   - ✅ 条件检查: `90.2 > 80.0` → 满足
   - 标签: `{"alert_name": "CPU使用率过高", "host_ip": "192.168.1.101", "value": "90.2", ...}`
   - 注释: `{"summary": "节点 192.168.1.101 CPU使用率过高", ...}`
   - 指纹: `"CPU使用率过高|host_ip=192.168.1.101"`
   - 状态: `Pending`
   - ID: `"alert_20250126_120000_456_7890"`

3. **AlertEngine 处理**:
   - 检查数据库中是否存在相同指纹的告警
   - 如果不存在，创建新告警
   - 如果存在 Pending 告警，检查是否满足 `for` 时长（5秒），满足则转为 Firing

## 关键设计点

### 1. 双重条件验证

- **SQL 层**: 在数据库查询时过滤数据，提高效率
- **应用层**: 再次验证条件，确保准确性

### 2. 指纹去重机制

- 相同指纹的告警被视为同一个告警实例
- 用于状态管理和去重
- 格式: `alert_name|key1=value1|key2=value2`

### 3. 状态生命周期

```
Pending → Firing → Resolved
   ↓         ↓         ↓
创建时间  触发时间  解决时间
```

### 4. 占位符替换

- 支持 `{{key}}` 格式的占位符
- 从 `labels` 中查找对应的值进行替换
- 用于动态生成告警消息

### 5. 时间戳管理

- `created_at`: 告警首次创建时间
- `starts_at`: 告警首次触发时间（Pending → Firing）
- `updated_at`: 告警更新时间（每次匹配时更新）
- `ends_at`: 告警解决时间（Firing → Resolved）

## 注意事项

1. **性能考虑**:
   - 大量告警规则可能导致性能问题
   - 建议优化 SQL 查询和索引

2. **数据一致性**:
   - 使用指纹确保相同告警的去重
   - 状态转换需要原子性操作

3. **错误处理**:
   - 条件检查失败时跳过该行
   - 数据库操作失败时记录日志

4. **扩展性**:
   - 标签和注释支持动态扩展
   - 占位符替换支持任意标签键

