# 告警事件重新触发逻辑修复

## 🔍 **问题描述**

在原始实现中，当一个告警事件已经被解决（`resolved`）后，如果同一个规则和节点再次触发告警，系统会更新旧的事件而不是创建新的事件。这导致：

1. **历史丢失**: 无法区分不同时间段的告警
2. **时间混乱**: `created_at` 和 `updated_at` 时间不准确
3. **状态混乱**: 可能覆盖已解决的事件状态

## 🔧 **修复方案**

### **修复前的问题逻辑**
```cpp
// 问题：只要事件存在就更新
bool event_exists = event_repo_->hasEvent(fingerprint);
if (event_exists) {
    ev = event_repo_->getEvent(fingerprint).value(); // 总是更新
} else {
    // 创建新事件
}
```

### **修复后的正确逻辑**
```cpp
// 修复：检查事件状态，只有非 resolved 状态才更新
bool event_exists = event_repo_->hasEvent(fingerprint);
bool should_update_existing = false;
AlertEvent ev;

if (event_exists) {
    ev = event_repo_->getEvent(fingerprint).value();
    // 关键：只有当事件状态不是 resolved 时才更新现有事件
    should_update_existing = (ev.status != "resolved");
}

if (!should_update_existing) {
    // 创建新事件（首次创建或重新触发）
    ev.fingerprint = fingerprint;
    ev.labels = pt.labels;
    ev.summary = rule.summary;
    ev.description = rule.description;
    ev.created_at = std::to_string(now_ms);
    ev.status = ""; // 清空状态，后续会设置
}
```

## 📊 **场景示例**

### **场景1: 首次触发告警**
```
时间: 12:00:00
状态: Inactive → Pending
结果: 创建新事件 (created_at: 12:00:00, status: "pending")
```

### **场景2: 持续告警**
```
时间: 12:00:30
状态: Pending → Firing
结果: 更新现有事件 (updated_at: 12:00:30, status: "firing")
```

### **场景3: 告警解决**
```
时间: 12:01:00
状态: Firing → Resolved
结果: 更新现有事件 (updated_at: 12:01:00, status: "resolved", ends_at: 12:01:00)
```

### **场景4: 重新触发告警**
```
时间: 12:05:00
状态: Resolved → Pending
结果: 创建新事件 (created_at: 12:05:00, status: "pending")
```

## 🎯 **修复效果**

### **修复前的问题**
```json
// 12:00:00 首次触发
{
  "fingerprint": "cpu_high|host_ip=192.168.1.100",
  "status": "pending",
  "created_at": "12:00:00",
  "updated_at": "12:00:00"
}

// 12:01:00 解决
{
  "fingerprint": "cpu_high|host_ip=192.168.1.100",
  "status": "resolved",
  "created_at": "12:00:00",  // 正确
  "updated_at": "12:01:00",
  "ends_at": "12:01:00"
}

// 12:05:00 重新触发 - 问题：更新了旧事件
{
  "fingerprint": "cpu_high|host_ip=192.168.1.100",
  "status": "pending",
  "created_at": "12:00:00",  // 错误：应该是 12:05:00
  "updated_at": "12:05:00",
  "ends_at": "12:01:00"      // 错误：应该清空
}
```

### **修复后的正确结果**
```json
// 12:00:00 首次触发
{
  "fingerprint": "cpu_high|host_ip=192.168.1.100",
  "status": "pending",
  "created_at": "12:00:00",
  "updated_at": "12:00:00"
}

// 12:01:00 解决
{
  "fingerprint": "cpu_high|host_ip=192.168.1.100",
  "status": "resolved",
  "created_at": "12:00:00",
  "updated_at": "12:01:00",
  "ends_at": "12:01:00"
}

// 12:05:00 重新触发 - 正确：创建新事件
{
  "fingerprint": "cpu_high|host_ip=192.168.1.100",
  "status": "pending",
  "created_at": "12:05:00",  // 正确：新的创建时间
  "updated_at": "12:05:00",
  "ends_at": ""              // 正确：新事件没有结束时间
}
```

## 🔄 **状态转换逻辑**

### **AlertState 状态转换**
```
Inactive ──(条件满足)──> Pending ──(持续满足)──> Firing
    ↑                                        ↓
    └──(条件不满足)──<── Resolved <──(条件不满足)──┘
```

### **AlertEvent 创建/更新逻辑**
```
1. 检查是否存在事件
   ├── 不存在 → 创建新事件
   └── 存在 → 检查状态
       ├── status != "resolved" → 更新现有事件
       └── status == "resolved" → 创建新事件
```

## 💡 **设计优势**

1. **历史完整性**: 每次告警周期都有独立的事件记录
2. **时间准确性**: `created_at` 反映真实的告警开始时间
3. **状态清晰**: 不会混淆不同时间段的告警状态
4. **查询友好**: 可以准确统计告警频率和持续时间

## 🎯 **总结**

修复后的逻辑确保了：
- **已解决的告警事件不会被更新**
- **重新触发的告警会创建新的事件**
- **每个告警周期都有完整的事件记录**
- **时间戳准确反映告警的生命周期**

这样设计更符合告警系统的预期行为，便于后续的告警分析和统计。
