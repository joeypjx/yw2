-- ============================================================================
-- 迁移脚本：添加 network 相关新字段
-- 用于更新现有数据库表结构，添加新增的 network 字段
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/migration_add_network_fields.sql
-- ============================================================================

BEGIN;

-- ============================================================================
-- 更新 resource_network 表
-- ============================================================================

-- 检查并添加 rx_drop_rate 字段
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'resource_network' AND column_name = 'rx_drop_rate'
    ) THEN
        ALTER TABLE resource_network ADD COLUMN rx_drop_rate DOUBLE PRECISION;
    END IF;
END $$;

-- 检查并添加 tx_drop_rate 字段
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'resource_network' AND column_name = 'tx_drop_rate'
    ) THEN
        ALTER TABLE resource_network ADD COLUMN tx_drop_rate DOUBLE PRECISION;
    END IF;
END $$;

-- 检查并添加 state 字段
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'resource_network' AND column_name = 'state'
    ) THEN
        ALTER TABLE resource_network ADD COLUMN state INTEGER;
    END IF;
END $$;

-- 如果 rx_rate 和 tx_rate 是 BIGINT，需要改为 DOUBLE PRECISION
-- 注意：PostgreSQL 不支持直接修改列类型从 BIGINT 到 DOUBLE PRECISION
-- 需要先添加新列，复制数据，删除旧列，重命名新列
DO $$
BEGIN
    -- 检查 rx_rate 类型
    IF EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'resource_network' 
        AND column_name = 'rx_rate' 
        AND data_type = 'bigint'
    ) THEN
        -- 添加新列
        ALTER TABLE resource_network ADD COLUMN rx_rate_new DOUBLE PRECISION;
        -- 复制数据
        UPDATE resource_network SET rx_rate_new = rx_rate::DOUBLE PRECISION;
        -- 删除旧列
        ALTER TABLE resource_network DROP COLUMN rx_rate;
        -- 重命名新列
        ALTER TABLE resource_network RENAME COLUMN rx_rate_new TO rx_rate;
    END IF;
END $$;

DO $$
BEGIN
    -- 检查 tx_rate 类型
    IF EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'resource_network' 
        AND column_name = 'tx_rate' 
        AND data_type = 'bigint'
    ) THEN
        -- 添加新列
        ALTER TABLE resource_network ADD COLUMN tx_rate_new DOUBLE PRECISION;
        -- 复制数据
        UPDATE resource_network SET tx_rate_new = tx_rate::DOUBLE PRECISION;
        -- 删除旧列
        ALTER TABLE resource_network DROP COLUMN tx_rate;
        -- 重命名新列
        ALTER TABLE resource_network RENAME COLUMN tx_rate_new TO tx_rate;
    END IF;
END $$;

-- ============================================================================
-- 更新 component_resource 表
-- ============================================================================

-- 检查并添加 net_rx_rate 字段
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'component_resource' AND column_name = 'net_rx_rate'
    ) THEN
        ALTER TABLE component_resource ADD COLUMN net_rx_rate DOUBLE PRECISION;
    END IF;
END $$;

-- 检查并添加 net_tx_rate 字段
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns 
        WHERE table_name = 'component_resource' AND column_name = 'net_tx_rate'
    ) THEN
        ALTER TABLE component_resource ADD COLUMN net_tx_rate DOUBLE PRECISION;
    END IF;
END $$;

COMMIT;

-- ============================================================================
-- 迁移完成
-- ============================================================================
-- 已添加以下字段：
--   - resource_network: rx_drop_rate, tx_drop_rate, state
--   - resource_network: rx_rate, tx_rate (类型从 BIGINT 改为 DOUBLE PRECISION)
--   - component_resource: net_rx_rate, net_tx_rate
-- ============================================================================

