-- 数据库迁移脚本：将timestamptz字段改为timestamp
-- 适用于中国本地时间，移除时区处理
-- 使用方法：psql "postgres://user:password@host:5432/dbname" -f migrate_timestamptz_to_timestamp.sql

-- 开始事务
BEGIN;

-- 1. 备份现有数据（可选）
-- CREATE TABLE alert_backup AS SELECT * FROM alert;
-- CREATE TABLE alert_rules_backup AS SELECT * FROM alert_rules;

-- 2. 更新alert表的时间字段
-- 将timestamptz字段转换为timestamp（移除时区信息）
ALTER TABLE alert 
    ALTER COLUMN created_at TYPE TIMESTAMP USING created_at::timestamp,
    ALTER COLUMN starts_at TYPE TIMESTAMP USING starts_at::timestamp,
    ALTER COLUMN updated_at TYPE TIMESTAMP USING updated_at::timestamp,
    ALTER COLUMN ends_at TYPE TIMESTAMP USING ends_at::timestamp;

-- 3. 更新alert_rules表的时间字段
ALTER TABLE alert_rules 
    ALTER COLUMN created_at TYPE TIMESTAMP USING created_at::timestamp,
    ALTER COLUMN updated_at TYPE TIMESTAMP USING updated_at::timestamp;

-- 4. 更新触发器函数中的NOW()函数
-- PostgreSQL的NOW()函数在timestamp类型下返回本地时间
-- 不需要修改，因为NOW()会自动适应字段类型

-- 5. 验证数据完整性
-- 检查时间字段是否正确转换
SELECT 
    'alert表时间字段检查' as table_name,
    COUNT(*) as total_records,
    MIN(created_at) as min_created_at,
    MAX(created_at) as max_created_at
FROM alert
UNION ALL
SELECT 
    'alert_rules表时间字段检查' as table_name,
    COUNT(*) as total_records,
    MIN(created_at) as min_created_at,
    MAX(created_at) as max_created_at
FROM alert_rules;

-- 6. 显示转换后的示例数据
SELECT 
    'alert表示例数据' as info,
    id,
    created_at,
    starts_at,
    updated_at,
    ends_at
FROM alert 
LIMIT 3;

SELECT 
    'alert_rules表示例数据' as info,
    id,
    alert_name,
    created_at,
    updated_at
FROM alert_rules 
LIMIT 3;

-- 提交事务
COMMIT;

-- 显示迁移完成信息
SELECT '数据库迁移完成：timestamptz -> timestamp' as migration_status;
