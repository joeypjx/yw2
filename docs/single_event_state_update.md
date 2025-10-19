# 单事件状态更新模式实现

## 概述

实现了你建议的优化方案：**只在第一次触发时产生新的 AlertEvent 并存储，之后只是改变这个 AlertEvent 的状态**。

## 🔄 **模式对比**

### ❌ **原模式：多事件模式**
```cpp
// 每次状态变化都创建新事件
if (状态变化) {
    AlertEvent ev;
    ev.status = "pending";  // 或 "firing", "resolved"
    event_repo_->append(ev);  // 每次都添加新事件
}
```

**问题：**
- 同一告警实例产生多个事件记录
- 存储冗余，内存浪费
- 查询复杂，需要按 fingerprint 分组
- 历史记录混乱

### ✅ **新模式：单事件状态更新**
```cpp
// 检查是否已存在事件
bool event_exists = event_repo_->hasEvent(fingerprint);
AlertEvent ev;

if (event_exists) {
    // 获取现有事件
    ev = event_repo_->getEvent(fingerprint).value();
} else {
    // 创建新事件
    ev.fingerprint = fingerprint;
    ev.created_at = std::to_string(now_ms);
}

// 更新状态
ev.status = "pending";  // 或 "firing", "resolved"
ev.updated_at = std::to_string(now_ms);

// 只有状态变化时才处理
if (status_changed) {
    if (event_exists) {
        event_repo_->updateEvent(ev);  // 更新现有事件
    } else {
        event_repo_->append(ev);       // 创建新事件
    }
}
```

## 🏗️ **实现架构**

### 1. **接口扩展**

在 `IEventRepository` 中添加新方法：
```cpp
class IEventRepository {
public:
    // 原有方法
    virtual bool append(const AlertEvent& event) = 0;
    virtual std::vector<AlertEvent> query(const std::string& duration) const = 0;
    virtual std::size_t countByStatus(AlertStatus status) const = 0;
    
    // 新增方法
    virtual bool hasEvent(const std::string& fingerprint) const = 0;
    virtual std::optional<AlertEvent> getEvent(const std::string& fingerprint) const = 0;
    virtual bool updateEvent(const AlertEvent& event) = 0;
};
```

### 2. **MemoryEventRepository 实现**

```cpp
bool MemoryEventRepository::hasEvent(const std::string& fingerprint) const {
    std::lock_guard<std::mutex> lk(mu_);
    return std::any_of(events_.begin(), events_.end(), [&fingerprint](const AlertEvent& event) {
        return event.fingerprint == fingerprint;
    });
}

std::optional<AlertEvent> MemoryEventRepository::getEvent(const std::string& fingerprint) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = std::find_if(events_.begin(), events_.end(), [&fingerprint](const AlertEvent& event) {
        return event.fingerprint == fingerprint;
    });
    return (it != events_.end()) ? std::optional<AlertEvent>(*it) : std::nullopt;
}

bool MemoryEventRepository::updateEvent(const AlertEvent& event) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = std::find_if(events_.begin(), events_.end(), [&event](const AlertEvent& e) {
        return e.fingerprint == event.fingerprint;
    });
    if (it != events_.end()) {
        *it = event;
        return true;
    }
    return false;
}
```

### 3. **DatabaseEventRepository 实现**

```cpp
bool DatabaseEventRepository::hasEvent(const std::string& fingerprint) const {
    const std::string sql = "SELECT 1 FROM alert_event WHERE fingerprint = $1 LIMIT 1";
    pqxx::result r = tx.exec_params(sql, fingerprint);
    return !r.empty();
}

std::optional<AlertEvent> DatabaseEventRepository::getEvent(const std::string& fingerprint) const {
    const std::string sql = R"(
        SELECT fingerprint, labels, status, summary, description, 
               starts_at, ends_at, created_at, updated_at
        FROM alert_event WHERE fingerprint = $1 LIMIT 1
    )";
    // 执行查询并解析结果...
}

bool DatabaseEventRepository::updateEvent(const AlertEvent& event) {
    const std::string sql = R"(
        UPDATE alert_event SET
            labels = $2, status = $3, summary = $4, description = $5,
            starts_at = $6, ends_at = $7, updated_at = $8
        WHERE fingerprint = $1
    )";
    // 执行更新...
}
```

## 📊 **状态变化流程**

### 完整生命周期示例

**场景：CPU使用率告警**

```
时间轴: 10:00 → 10:01 → 10:02 → 10:03 → 10:04 → 10:05
CPU值:   70% →   85% →   90% →   95% →   80% →   75%
状态:  Inactive → Pending → Firing → Firing → Resolved → Inactive
事件:     无   →  创建   →  更新   →  无   →  更新   →   无
```

**详细过程：**

1. **10:00 - 初始状态**
   - `AlertState.status = Inactive`
   - 无事件

2. **10:01 - 首次触发**
   - `AlertState.status = Pending`
   - 创建 `AlertEvent`：`{fingerprint: "cpu_high|host_ip=192.168.1.100", status: "pending"}`

3. **10:02 - 进入告警**
   - `AlertState.status = Firing`
   - 更新 `AlertEvent`：`{status: "firing"}`

4. **10:03 - 持续告警**
   - `AlertState.status = Firing`
   - 无事件变化

5. **10:04 - 恢复**
   - `AlertState.status = Resolved`
   - 更新 `AlertEvent`：`{status: "resolved", ends_at: "10:04"}`

6. **10:05 - 完全恢复**
   - `AlertState.status = Inactive`
   - 无事件变化

## 🎯 **优化效果**

### 1. **存储优化**
- **事件数量：** 从 N 个减少到 1 个（每个告警实例）
- **内存使用：** 大幅减少重复数据
- **存储空间：** 数据库存储更高效

### 2. **查询优化**
- **简单查询：** 直接按 fingerprint 查询，无需分组
- **状态获取：** 一次查询即可获得最新状态
- **历史追踪：** 通过 `updated_at` 字段追踪状态变化

### 3. **性能提升**
- **减少 I/O：** 更新比插入更高效
- **减少网络：** 数据库操作更少
- **减少序列化：** JSON 处理更少

## 📝 **使用示例**

### 查询告警状态
```cpp
// 检查告警是否存在
if (event_repo->hasEvent("cpu_high|host_ip=192.168.1.100")) {
    // 获取告警详情
    auto event = event_repo->getEvent("cpu_high|host_ip=192.168.1.100");
    std::cout << "告警状态: " << event->status << std::endl;
    std::cout << "创建时间: " << event->created_at << std::endl;
    std::cout << "更新时间: " << event->updated_at << std::endl;
}
```

### 更新告警状态
```cpp
// 获取现有事件
auto event = event_repo->getEvent(fingerprint);
if (event) {
    // 更新状态
    event->status = "firing";
    event->updated_at = std::to_string(now_ms);
    event_repo->updateEvent(*event);
}
```

## ⚠️ **注意事项**

### 1. **数据一致性**
- 确保 `AlertState` 和 `AlertEvent` 状态同步
- 使用事务保证原子性操作

### 2. **历史记录**
- 如果需要完整的历史记录，可以考虑添加 `AlertEventHistory` 表
- 或者使用事件溯源模式

### 3. **并发安全**
- 所有操作都是线程安全的
- 使用互斥锁保护共享数据

## 🔧 **配置建议**

### 数据库索引
```sql
-- 为 fingerprint 创建唯一索引
CREATE UNIQUE INDEX idx_alert_event_fingerprint ON alert_event(fingerprint);

-- 为状态查询创建索引
CREATE INDEX idx_alert_event_status ON alert_event(status);
```

### 内存优化
```cpp
// 使用 MemoryEventRepository 时设置合理的容量
auto event_repo = std::make_shared<MemoryEventRepository>(5000, 24 * 60 * 60 * 1000);
```

## 总结

单事件状态更新模式是一个很好的优化方案，它：

- **减少了存储冗余**
- **提高了查询效率**
- **简化了状态管理**
- **保持了功能完整性**

这个实现既满足了告警系统的核心需求，又大大提升了性能和可维护性！
