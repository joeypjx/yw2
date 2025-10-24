# AlertV2 时间格式统一化更新

## 概述
将AlertV2系统的时间处理从UTC时区改为中国本地时间（CST），移除复杂的时区转换逻辑，简化系统架构。

## 主要更改

### 1. 数据库Schema更新

#### alert_setup.sql
- `created_at`: `TIMESTAMPTZ` → `TIMESTAMP`
- `starts_at`: `TIMESTAMPTZ` → `TIMESTAMP`
- `updated_at`: `TIMESTAMPTZ` → `TIMESTAMP`
- `ends_at`: `TIMESTAMPTZ` → `TIMESTAMP`

#### alert_rule_setup_fixed.sql
- `created_at`: `TIMESTAMPTZ` → `TIMESTAMP`
- `updated_at`: `TIMESTAMPTZ` → `TIMESTAMP`

### 2. 代码更新

#### AlertRepository.cpp
- SQL语句中的类型转换：`::timestamptz` → `::timestamp`

#### Alert.cpp
- `setCreatedNow()`: 使用`std::localtime`替代`std::gmtime`，移除`Z`后缀
- `setUpdatedNow()`: 使用`std::localtime`替代`std::gmtime`，移除`Z`后缀
- `setStartsNow()`: 使用`std::localtime`替代`std::gmtime`，移除`Z`后缀
- `setEndsNow()`: 使用`std::localtime`替代`std::gmtime`，移除`Z`后缀

#### AlertRule.cpp
- `setCreatedNow()`: 使用`std::localtime`替代`std::gmtime`，移除`Z`后缀
- `setUpdatedNow()`: 使用`std::localtime`替代`std::gmtime`，移除`Z`后缀

#### AlertEngine.cpp
- `parseISOTime()`: 大幅简化，移除复杂的时区转换逻辑
- 移除PostgreSQL时区格式处理
- 移除UTC/本地时区偏移计算

#### AlertEngine.h
- 更新`parseISOTime()`方法注释，反映新的本地时间处理方式

### 3. 时间格式变化

#### 之前（UTC时区）
```
2024-01-01T12:00:00.123Z
2025-10-24 01:27:39.398+00
```

#### 现在（本地时间）
```
2024-01-01T12:00:00.123
2025-10-24 01:27:39.398
```

## 优势

1. **简化逻辑**：移除复杂的时区转换代码
2. **提高性能**：减少不必要的时区计算
3. **用户友好**：显示的时间就是用户看到的时间
4. **减少错误**：避免时区转换导致的bug
5. **维护简单**：代码更清晰，易于理解和维护

## 注意事项

1. **仅适用于中国**：此更改假设系统只在中国使用
2. **数据迁移**：现有数据库需要使用迁移脚本更新
3. **时间同步**：确保服务器时间准确
4. **向后兼容**：新代码仍能解析带`Z`后缀的时间字符串

## 迁移步骤

1. **备份数据**：在迁移前备份现有数据
2. **运行迁移脚本**：执行`migrate_timestamptz_to_timestamp.sql`
3. **更新代码**：部署新的AlertV2代码
4. **验证功能**：测试告警创建、更新和查询功能

## 文件清单

### 更新的文件
- `docs/alert_setup.sql`
- `docs/alert_rule_setup_fixed.sql`
- `src/alertv2/infrastructure/AlertRepository.cpp`
- `src/alertv2/domain/Alert.cpp`
- `src/alertv2/domain/AlertRule.cpp`
- `src/alertv2/application/AlertEngine.cpp`
- `src/alertv2/application/AlertEngine.h`

### 新增的文件
- `docs/migrate_timestamptz_to_timestamp.sql` - 数据库迁移脚本
- `docs/timezone_migration_summary.md` - 本总结文档

## 测试建议

1. **时间生成测试**：验证新创建的时间格式正确
2. **时间解析测试**：验证时间字符串解析功能
3. **数据库操作测试**：验证告警的增删改查功能
4. **时区一致性测试**：确保所有时间字段使用相同的时区
