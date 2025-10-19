# MemoryEventRepository 使用说明

## 概述

`MemoryEventRepository` 现在支持最大容量限制和超时删除机制，防止内存无限增长。

## 主要特性

### 1. 容量限制
- **默认最大事件数：** 10,000 个
- **自动清理：** 超过容量时自动删除最旧的事件
- **预分配：** 构造函数中预分配内存空间

### 2. 超时删除
- **默认保存时间：** 7 天
- **自动清理：** 定期删除过期事件
- **时间戳解析：** 基于 `created_at` 字段判断事件年龄

### 3. 定期清理
- **清理频率：** 每小时执行一次
- **手动清理：** 支持手动调用 `cleanup()` 方法
- **统计信息：** 提供当前大小和配置信息

## 使用方法

### 1. 创建实例

```cpp
// 使用默认配置（10000个事件，7天）
auto event_repo = std::make_shared<MemoryEventRepository>();

// 自定义配置（5000个事件，3天）
auto event_repo = std::make_shared<MemoryEventRepository>(5000, 3 * 24 * 60 * 60 * 1000);
```

### 2. 在 AlertManager 中使用

```cpp
// 替换 DatabaseEventRepository
event_repo_ = std::make_shared<MemoryEventRepository>(10000, 7 * 24 * 60 * 60 * 1000);
```

### 3. 手动清理

```cpp
// 手动触发清理
event_repo->cleanup();

// 获取统计信息
std::size_t current_size = event_repo->size();
std::size_t max_events = event_repo->getMaxEvents();
std::int64_t max_age_ms = event_repo->getMaxAgeMs();
```

## 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `max_events` | 10,000 | 最大事件数量 |
| `max_age_ms` | 7天 | 最大保存时间（毫秒） |

## 清理策略

### 1. 容量清理
- 当事件数量达到 `max_events` 时触发
- 先清理过期事件，再删除最旧的事件
- 确保新事件能够成功添加

### 2. 时间清理
- 基于 `created_at` 字段判断事件年龄
- 删除超过 `max_age_ms` 的事件
- 解析失败的事件会被保留

### 3. 定期清理
- 每小时自动执行一次
- 通过 `AlertManager` 的调度器管理
- 支持手动触发

## 性能优化

### 1. 内存预分配
```cpp
events_.reserve(max_events_); // 预分配空间，减少重新分配
```

### 2. 批量删除
```cpp
// 使用 std::remove_if + erase 批量删除过期事件
events_.erase(
    std::remove_if(events_.begin(), events_.end(), predicate),
    events_.end()
);
```

### 3. 线程安全
- 使用 `std::lock_guard<std::mutex>` 保护共享数据
- 所有操作都是线程安全的

## 监控和调试

### 1. 日志记录
```cpp
spdlog::debug("Cleaned up expired events, current size: {}", mem_repo->size());
```

### 2. 统计信息
```cpp
// 获取当前状态
std::size_t size = event_repo->size();
std::size_t max_events = event_repo->getMaxEvents();
std::int64_t max_age_ms = event_repo->getMaxAgeMs();
```

### 3. 健康检查
```cpp
// 检查是否接近容量限制
if (event_repo->size() > event_repo->getMaxEvents() * 0.9) {
    spdlog::warn("Event repository is near capacity: {}/{}", 
                 event_repo->size(), event_repo->getMaxEvents());
}
```

## 注意事项

1. **时间戳格式：** 确保 `created_at` 字段是有效的毫秒时间戳字符串
2. **内存使用：** 根据实际需求调整 `max_events` 参数
3. **清理频率：** 根据事件产生频率调整清理间隔
4. **数据持久化：** 内存存储的数据在程序重启后会丢失

## 示例代码

```cpp
#include "AlertServices.h"

int main() {
    // 创建事件仓库
    auto event_repo = std::make_shared<MemoryEventRepository>(5000, 24 * 60 * 60 * 1000); // 5000个事件，1天
    
    // 添加事件
    AlertEvent event;
    event.fingerprint = "test_fingerprint";
    event.status = "firing";
    event.summary = "Test alert";
    event.created_at = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    
    event_repo->append(event);
    
    // 查询事件
    auto events = event_repo->query("1h");
    std::cout << "Total events: " << events.size() << std::endl;
    
    // 手动清理
    event_repo->cleanup();
    std::cout << "After cleanup: " << event_repo->size() << std::endl;
    
    return 0;
}
```
