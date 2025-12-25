# TimescaleDB 压缩和保留策略文档

本文档详细说明 TimescaleDB 的压缩策略（Compression Policy）和保留策略（Retention Policy）的概念、原理、配置和使用方法。

## 概述

TimescaleDB 是 PostgreSQL 的时序数据库扩展，专门用于处理时序数据。为了优化存储和查询性能，TimescaleDB 提供了两个重要的自动化策略：

1. **压缩策略（Compression Policy）**：自动压缩旧数据，减少存储空间
2. **保留策略（Retention Policy）**：自动删除超过保留期的数据，控制数据量

## 一、压缩策略（Compression Policy）

### 1.1 什么是压缩策略

压缩策略是 TimescaleDB 的一个自动化功能，它会定期将超过指定时间的数据块（chunks）从**未压缩格式**转换为**压缩格式**。

**工作原理**：
- TimescaleDB 将数据按时间分区存储在多个 chunks（数据块）中
- 新数据写入时保持未压缩状态，便于快速写入和查询
- 当数据超过指定时间（如 7 天）后，自动压缩这些 chunks
- 压缩后的数据占用更少的存储空间，但查询性能略有下降

### 1.2 压缩的优势

1. **节省存储空间**：压缩比通常可达 **90% 以上**（取决于数据类型）
2. **降低存储成本**：减少磁盘使用量
3. **提高查询性能**：压缩后的数据块更小，I/O 操作更少
4. **自动管理**：无需手动干预，系统自动执行

### 1.3 压缩配置

在 `docs/database_setup.sql` 中，压缩配置分为两个步骤：

#### 步骤 1：启用压缩并设置压缩参数

```sql
ALTER TABLE resource_cpu SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);
```

**参数说明**：
- `timescaledb.compress = true`：启用压缩功能
- `timescaledb.compress_orderby`：指定压缩时的排序顺序，影响压缩效率和查询性能
- `timescaledb.compress_segmentby`：指定分段键，相同值的行会被分组在一起（可选）

**示例配置**：

```sql
-- 简单表（无标签列）
ALTER TABLE resource_cpu SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

-- 有标签列的表（如 network、disk、gpu）
ALTER TABLE resource_network SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip',
  timescaledb.compress_segmentby = 'interface'  -- 按接口分组
);
```

**为什么需要 `compress_orderby` 和 `compress_segmentby`**：
- `compress_orderby`：指定压缩时数据的排序方式，通常与查询的 ORDER BY 一致，提高查询效率
- `compress_segmentby`：将相同值的行分组，提高压缩比和查询性能（特别是按标签列过滤时）

#### 步骤 2：添加压缩策略

```sql
SELECT add_compression_policy('resource_cpu', INTERVAL '7 days', if_not_exists => TRUE);
```

**参数说明**：
- `'resource_cpu'`：表名
- `INTERVAL '7 days'`：数据超过 7 天后自动压缩
- `if_not_exists => TRUE`：如果策略已存在，不报错

**工作原理**：
- TimescaleDB 后台任务会定期检查每个 chunk
- 如果 chunk 中最新的数据已经超过 7 天，则压缩该 chunk
- 压缩是渐进式的，不会一次性压缩所有旧数据

### 1.4 压缩策略配置示例

在 `docs/database_setup.sql` 中的配置：

```sql
-- 资源监控表：7 天后压缩
SELECT add_compression_policy('resource_cpu',       INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_memory',    INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_network',   INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_disk',      INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_gpu',       INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('component_resource', INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('resource_alive',     INTERVAL '7 days', if_not_exists => TRUE);

-- BMC 表：7 天后压缩
SELECT add_compression_policy('bmc_fan',    INTERVAL '7 days', if_not_exists => TRUE);
SELECT add_compression_policy('bmc_sensor', INTERVAL '7 days', if_not_exists => TRUE);
```

**为什么选择 7 天**：
- 7 天内的数据通常是热数据，需要频繁查询和更新
- 7 天后的数据变为温数据，查询频率降低，适合压缩
- 平衡了存储空间和查询性能

### 1.5 压缩策略管理

**查看压缩策略**：
```sql
SELECT * FROM timescaledb_information.job_stats 
WHERE proc_name = 'policy_compression';
```

**查看压缩状态**：
```sql
SELECT 
  chunk_schema,
  chunk_name,
  range_start,
  range_end,
  is_compressed,
  uncompressed_heap_size,
  uncompressed_toast_size,
  compressed_heap_size,
  compressed_toast_size
FROM timescaledb_information.chunks
WHERE hypertable_name = 'resource_cpu'
ORDER BY range_start DESC;
```

**手动压缩**（如果需要）：
```sql
SELECT compress_chunk('_timescaledb_internal._hyper_1_1_chunk');
```

**删除压缩策略**：
```sql
SELECT remove_compression_policy('resource_cpu', if_exists => TRUE);
```

## 二、保留策略（Retention Policy）

### 2.1 什么是保留策略

保留策略是 TimescaleDB 的另一个自动化功能，它会定期删除超过指定保留期的数据块（chunks），从而控制数据库的大小。

**工作原理**：
- TimescaleDB 后台任务定期检查每个 chunk
- 如果 chunk 中最新的数据已经超过保留期，则删除整个 chunk
- 删除是按 chunk 进行的，不是按行删除（更高效）

### 2.2 保留策略的优势

1. **自动清理**：无需手动删除旧数据
2. **控制数据量**：防止数据库无限增长
3. **节省存储**：自动删除不再需要的数据
4. **提高性能**：减少数据量，提高查询速度

### 2.3 保留策略配置

在 `docs/database_setup.sql` 中的配置：

```sql
-- 资源监控表：90 天后删除
SELECT add_retention_policy('resource_cpu',       INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_memory',    INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_network',   INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_disk',      INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_gpu',       INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('component_resource', INTERVAL '90 days', if_not_exists => TRUE);
SELECT add_retention_policy('resource_alive',     INTERVAL '90 days', if_not_exists => TRUE);

-- BMC 表：不同的保留期
SELECT add_retention_policy('bmc_fan',    INTERVAL '30 days', if_not_exists => TRUE);  -- 30 天
SELECT add_retention_policy('bmc_sensor', INTERVAL '90 days', if_not_exists => TRUE); -- 90 天
```

**保留期选择**：
- **资源监控表（90 天）**：通常需要较长的历史数据用于分析和趋势预测
- **BMC 风扇表（30 天）**：风扇数据变化频率高，保留期较短
- **BMC 传感器表（90 天）**：传感器数据可能用于长期分析

### 2.4 保留策略的工作原理

```
时间线：
今天 ──────────────────────────────────────────> 未来
      │
      ├─ 0-7天：未压缩数据（热数据）
      ├─ 7-90天：压缩数据（温数据）
      └─ 90天以上：已删除（冷数据）
```

**执行流程**：
1. TimescaleDB 后台任务定期运行（默认每小时）
2. 检查每个 chunk 的时间范围
3. 如果 chunk 的最新数据超过保留期，删除整个 chunk
4. 删除操作是原子性的，不会影响正在进行的查询

### 2.5 保留策略管理

**查看保留策略**：
```sql
SELECT * FROM timescaledb_information.job_stats 
WHERE proc_name = 'policy_retention';
```

**查看数据保留情况**：
```sql
SELECT 
  chunk_schema,
  chunk_name,
  range_start,
  range_end,
  NOW() - range_end as age
FROM timescaledb_information.chunks
WHERE hypertable_name = 'resource_cpu'
ORDER BY range_start DESC;
```

**手动删除旧数据**（如果需要）：
```sql
-- 删除 90 天前的数据
SELECT drop_chunks('resource_cpu', INTERVAL '90 days');
```

**删除保留策略**：
```sql
SELECT remove_retention_policy('resource_cpu', if_exists => TRUE);
```

## 三、压缩和保留策略的配合使用

### 3.1 数据生命周期

在 `docs/database_setup.sql` 中的典型配置：

```
数据生命周期：
┌─────────────────────────────────────────────────────────────┐
│ 0-7天：未压缩数据（热数据）                                    │
│   - 频繁写入和查询                                            │
│   - 保持未压缩状态，确保最佳性能                                │
├─────────────────────────────────────────────────────────────┤
│ 7-90天：压缩数据（温数据）                                     │
│   - 查询频率降低                                              │
│   - 已压缩，节省存储空间                                        │
│   - 查询性能略有下降，但仍在可接受范围                          │
├─────────────────────────────────────────────────────────────┤
│ 90天以上：已删除（冷数据）                                     │
│   - 自动删除，释放存储空间                                      │
│   - 如果需要长期保留，可以调整保留策略或导出到归档存储          │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 配置示例

**资源监控表**：
```sql
-- 1. 启用压缩
ALTER TABLE resource_cpu SET (
  timescaledb.compress = true,
  timescaledb.compress_orderby = 'time DESC, host_ip'
);

-- 2. 添加压缩策略：7 天后压缩
SELECT add_compression_policy('resource_cpu', INTERVAL '7 days', if_not_exists => TRUE);

-- 3. 添加保留策略：90 天后删除
SELECT add_retention_policy('resource_cpu', INTERVAL '90 days', if_not_exists => TRUE);
```

**BMC 风扇表**（保留期较短）：
```sql
-- 1. 启用压缩
ALTER TABLE bmc_fan SET (
  timescaledb.compress,
  timescaledb.compress_segmentby = 'boxid',
  timescaledb.compress_orderby = 'time DESC'
);

-- 2. 添加压缩策略：7 天后压缩
SELECT add_compression_policy('bmc_fan', INTERVAL '7 days', if_not_exists => TRUE);

-- 3. 添加保留策略：30 天后删除（风扇数据保留期较短）
SELECT add_retention_policy('bmc_fan', INTERVAL '30 days', if_not_exists => TRUE);
```

### 3.3 策略调整建议

**根据业务需求调整**：

1. **如果存储空间充足**：
   - 可以延长保留期（如 180 天或 365 天）
   - 可以缩短压缩时间（如 3 天）

2. **如果存储空间紧张**：
   - 可以缩短保留期（如 60 天）
   - 可以提前压缩（如 3 天）

3. **如果查询性能要求高**：
   - 可以延长未压缩期（如 14 天）
   - 可以调整 `compress_orderby` 以匹配查询模式

4. **如果需要长期归档**：
   - 可以禁用保留策略
   - 或者导出数据到外部存储（如对象存储）

## 四、注意事项

### 4.1 压缩策略注意事项

1. **压缩是不可逆的**：
   - 压缩后的数据无法直接修改
   - 如果需要修改，需要先解压缩

2. **压缩会影响写入性能**：
   - 压缩过程会消耗 CPU 和 I/O 资源
   - 建议在低峰期执行压缩

3. **查询性能**：
   - 压缩后的数据查询性能略有下降
   - 但通过合理的 `compress_orderby` 和 `compress_segmentby` 可以优化

4. **压缩比**：
   - 数值类型压缩比高（90%+）
   - 文本类型压缩比低（50-70%）
   - JSONB 类型压缩比中等（70-80%）

### 4.2 保留策略注意事项

1. **删除是不可逆的**：
   - 删除的数据无法恢复（除非有备份）
   - 删除前请确认数据不再需要

2. **删除是按 chunk 进行的**：
   - 如果 chunk 中部分数据在保留期内，部分在保留期外，整个 chunk 仍会被删除
   - 这是 TimescaleDB 的设计，为了保持 chunk 的完整性

3. **删除时机**：
   - 删除操作在后台自动执行
   - 默认每小时检查一次
   - 可以通过 `timescaledb.max_background_workers` 调整并发度

4. **数据导出**：
   - 如果需要长期保留数据，建议在删除前导出到外部存储
   - 可以使用 `COPY` 命令或 `pg_dump` 导出

### 4.3 最佳实践

1. **监控策略执行情况**：
   ```sql
   -- 查看压缩任务统计
   SELECT * FROM timescaledb_information.job_stats 
   WHERE proc_name IN ('policy_compression', 'policy_retention');
   ```

2. **定期检查存储使用情况**：
   ```sql
   -- 查看表大小
   SELECT 
     schemaname,
     tablename,
     pg_size_pretty(pg_total_relation_size(schemaname||'.'||tablename)) AS size
   FROM pg_tables
   WHERE schemaname = 'public'
   ORDER BY pg_total_relation_size(schemaname||'.'||tablename) DESC;
   ```

3. **根据实际使用情况调整策略**：
   - 监控查询模式
   - 监控存储使用情况
   - 根据业务需求调整保留期和压缩时间

4. **备份重要数据**：
   - 在删除前备份重要数据
   - 使用连续归档或定期备份

## 五、总结

压缩和保留策略是 TimescaleDB 管理时序数据的核心功能：

- **压缩策略**：自动压缩旧数据，节省存储空间（90%+），7 天后压缩
- **保留策略**：自动删除超期数据，控制数据量，90 天后删除（资源表）或 30 天后删除（BMC 风扇表）

**配置原则**：
- 根据数据访问模式选择压缩时间（热数据保持未压缩）
- 根据业务需求选择保留期（平衡存储成本和数据价值）
- 定期监控和调整策略，确保最佳性能

**在 `docs/database_setup.sql` 中的配置**：
- 所有资源监控表：7 天压缩，90 天保留
- BMC 风扇表：7 天压缩，30 天保留
- BMC 传感器表：7 天压缩，90 天保留

这些配置可以根据实际业务需求进行调整。

