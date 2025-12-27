-- ============================================================================
-- 插入 100 个不同类型的告警规则
-- 
-- 根据 docs/alert/database_schema.md 文档生成
-- 覆盖 CPU、内存、磁盘、网络、GPU、节点存活等不同类型的告警规则
-- 
-- 使用方法：
-- psql "postgres://user:password@host:5432/dbname" -f docs/alert/insert_100_alert_rules.sql
-- ============================================================================

-- 开始事务
BEGIN;

-- ============================================================================
-- CPU 相关告警规则（20个）
-- ============================================================================

-- CPU 使用率告警
INSERT INTO alert_rule (id, alert_name, expression, for_duration, severity, summary, description, alert_type, enabled) VALUES
('rule_cpu_usage_high_1', 'CPU使用率过高-严重', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 90}], "tags": []}'::jsonb,
 '30s', '严重', 'CPU使用率超过90%', '节点 {{host_ip}} 的CPU使用率已达到 {{value}}%，超过严重告警阈值90%，可能导致系统响应缓慢。', '硬件状态', true),

('rule_cpu_usage_high_2', 'CPU使用率过高-警告', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 80}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU使用率超过80%', '节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过警告阈值80%，请关注系统负载。', '硬件状态', true),

('rule_cpu_usage_high_3', 'CPU使用率过高-一般', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 70}], "tags": []}'::jsonb,
 '2m', '一般', 'CPU使用率超过70%', '节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过一般告警阈值70%。', '硬件状态', true),

('rule_cpu_usage_low', 'CPU使用率异常低', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": "<", "threshold": 5}], "tags": []}'::jsonb,
 '5m', '一般', 'CPU使用率低于5%', '节点 {{host_ip}} 的CPU使用率为 {{value}}%，异常偏低，可能存在系统异常。', '硬件状态', true),

-- CPU 温度告警
('rule_cpu_temp_critical', 'CPU温度严重过高', 
 '{"stable": "cpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 85}], "tags": []}'::jsonb,
 '30s', '严重', 'CPU温度超过85°C', '节点 {{host_ip}} 的CPU温度已达到 {{value}}°C，超过严重告警阈值85°C，存在过热风险。', '硬件状态', true),

('rule_cpu_temp_high', 'CPU温度过高', 
 '{"stable": "cpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 75}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU温度超过75°C', '节点 {{host_ip}} 的CPU温度为 {{value}}°C，超过警告阈值75°C，请检查散热系统。', '硬件状态', true),

('rule_cpu_temp_low', 'CPU温度异常低', 
 '{"stable": "cpu", "metric": "temperature", "conditions": [{"operator": "<", "threshold": 20}], "tags": []}'::jsonb,
 '2m', '一般', 'CPU温度低于20°C', '节点 {{host_ip}} 的CPU温度为 {{value}}°C，异常偏低，可能存在传感器故障。', '硬件状态', true),

-- CPU 负载告警
('rule_cpu_load_1m_high', 'CPU 1分钟负载过高', 
 '{"stable": "cpu", "metric": "load_avg_1m", "conditions": [{"operator": ">", "threshold": 10}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU 1分钟平均负载超过10', '节点 {{host_ip}} 的CPU 1分钟平均负载为 {{value}}，超过警告阈值10。', '硬件状态', true),

('rule_cpu_load_5m_high', 'CPU 5分钟负载过高', 
 '{"stable": "cpu", "metric": "load_avg_5m", "conditions": [{"operator": ">", "threshold": 8}], "tags": []}'::jsonb,
 '2m', '警告', 'CPU 5分钟平均负载超过8', '节点 {{host_ip}} 的CPU 5分钟平均负载为 {{value}}，超过警告阈值8。', '硬件状态', true),

('rule_cpu_load_15m_high', 'CPU 15分钟负载过高', 
 '{"stable": "cpu", "metric": "load_avg_15m", "conditions": [{"operator": ">", "threshold": 6}], "tags": []}'::jsonb,
 '3m', '警告', 'CPU 15分钟平均负载超过6', '节点 {{host_ip}} 的CPU 15分钟平均负载为 {{value}}，超过警告阈值6。', '硬件状态', true),

-- CPU 功耗告警
('rule_cpu_power_high', 'CPU功耗过高', 
 '{"stable": "cpu", "metric": "power", "conditions": [{"operator": ">", "threshold": 100}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU功耗超过100W', '节点 {{host_ip}} 的CPU功耗为 {{value}}W，超过警告阈值100W。', '硬件状态', true),

('rule_cpu_power_low', 'CPU功耗异常低', 
 '{"stable": "cpu", "metric": "power", "conditions": [{"operator": "<", "threshold": 5}], "tags": []}'::jsonb,
 '2m', '一般', 'CPU功耗低于5W', '节点 {{host_ip}} 的CPU功耗为 {{value}}W，异常偏低，可能存在系统异常。', '硬件状态', true),

-- CPU 核心分配告警
('rule_cpu_core_allocated_high', 'CPU核心分配率过高', 
 '{"stable": "cpu", "metric": "core_allocated", "conditions": [{"operator": ">=", "threshold": 8}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU核心分配率达到上限', '节点 {{host_ip}} 的CPU核心分配数为 {{value}}，已达到或接近上限。', '硬件状态', true),

-- ============================================================================
-- 内存相关告警规则（15个）
-- ============================================================================

('rule_memory_usage_critical', '内存使用率严重过高', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 95}], "tags": []}'::jsonb,
 '30s', '严重', '内存使用率超过95%', '节点 {{host_ip}} 的内存使用率已达到 {{value}}%，超过严重告警阈值95%，可能导致系统OOM。', '硬件状态', true),

('rule_memory_usage_high_1', '内存使用率过高-严重', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 90}], "tags": []}'::jsonb,
 '1m', '严重', '内存使用率超过90%', '节点 {{host_ip}} 的内存使用率为 {{value}}%，超过严重告警阈值90%，请及时处理。', '硬件状态', true),

('rule_memory_usage_high_2', '内存使用率过高-警告', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 80}], "tags": []}'::jsonb,
 '2m', '警告', '内存使用率超过80%', '节点 {{host_ip}} 的内存使用率为 {{value}}%，超过警告阈值80%。', '硬件状态', true),

('rule_memory_usage_high_3', '内存使用率过高-一般', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 70}], "tags": []}'::jsonb,
 '3m', '一般', '内存使用率超过70%', '节点 {{host_ip}} 的内存使用率为 {{value}}%，超过一般告警阈值70%。', '硬件状态', true),

('rule_memory_usage_low', '内存使用率异常低', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": "<", "threshold": 10}], "tags": []}'::jsonb,
 '5m', '一般', '内存使用率低于10%', '节点 {{host_ip}} 的内存使用率为 {{value}}%，异常偏低。', '硬件状态', true),

('rule_memory_free_low_1', '内存空闲量严重不足', 
 '{"stable": "memory", "metric": "free", "conditions": [{"operator": "<", "threshold": 1073741824}], "tags": []}'::jsonb,
 '30s', '严重', '内存空闲量低于1GB', '节点 {{host_ip}} 的内存空闲量仅为 {{value}} 字节（约 {{value}}GB），低于严重告警阈值1GB。', '硬件状态', true),

('rule_memory_free_low_2', '内存空闲量不足', 
 '{"stable": "memory", "metric": "free", "conditions": [{"operator": "<", "threshold": 2147483648}], "tags": []}'::jsonb,
 '1m', '警告', '内存空闲量低于2GB', '节点 {{host_ip}} 的内存空闲量为 {{value}} 字节（约 {{value}}GB），低于警告阈值2GB。', '硬件状态', true),

('rule_memory_used_high', '内存使用量过高', 
 '{"stable": "memory", "metric": "used", "conditions": [{"operator": ">", "threshold": 14073748835532}], "tags": []}'::jsonb,
 '1m', '警告', '内存使用量超过12TB', '节点 {{host_ip}} 的内存使用量为 {{value}} 字节，超过警告阈值。', '硬件状态', true),

-- ============================================================================
-- 磁盘相关告警规则（20个）
-- ============================================================================

('rule_disk_usage_critical', '磁盘使用率严重过高', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 95}], "tags": []}'::jsonb,
 '30s', '严重', '磁盘使用率超过95%', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用率已达到 {{value}}%，超过严重告警阈值95%，磁盘空间即将耗尽。', '硬件状态', true),

('rule_disk_usage_high_1', '磁盘使用率过高-严重', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 90}], "tags": []}'::jsonb,
 '1m', '严重', '磁盘使用率超过90%', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用率为 {{value}}%，超过严重告警阈值90%。', '硬件状态', true),

('rule_disk_usage_high_2', '磁盘使用率过高-警告', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 85}], "tags": []}'::jsonb,
 '2m', '警告', '磁盘使用率超过85%', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用率为 {{value}}%，超过警告阈值85%。', '硬件状态', true),

('rule_disk_usage_high_3', '磁盘使用率过高-一般', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 80}], "tags": []}'::jsonb,
 '3m', '一般', '磁盘使用率超过80%', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用率为 {{value}}%，超过一般告警阈值80%。', '硬件状态', true),

('rule_disk_free_low_1', '磁盘空闲空间严重不足', 
 '{"stable": "disk", "metric": "free", "conditions": [{"operator": "<", "threshold": 10737418240}], "tags": []}'::jsonb,
 '30s', '严重', '磁盘空闲空间低于10GB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）空闲空间仅为 {{value}} 字节（约 {{value}}GB），低于严重告警阈值10GB。', '硬件状态', true),

('rule_disk_free_low_2', '磁盘空闲空间不足', 
 '{"stable": "disk", "metric": "free", "conditions": [{"operator": "<", "threshold": 21474836480}], "tags": []}'::jsonb,
 '1m', '警告', '磁盘空闲空间低于20GB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）空闲空间为 {{value}} 字节（约 {{value}}GB），低于警告阈值20GB。', '硬件状态', true),

('rule_disk_free_low_3', '磁盘空闲空间偏低', 
 '{"stable": "disk", "metric": "free", "conditions": [{"operator": "<", "threshold": 53687091200}], "tags": []}'::jsonb,
 '2m', '一般', '磁盘空闲空间低于50GB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）空闲空间为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值50GB。', '硬件状态', true),

('rule_disk_used_high', '磁盘使用量过高', 
 '{"stable": "disk", "metric": "used", "conditions": [{"operator": ">", "threshold": 1099511627776}], "tags": []}'::jsonb,
 '1m', '警告', '磁盘使用量超过1TB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用量为 {{value}} 字节，超过警告阈值。', '硬件状态', true),

-- 针对特定挂载点的告警
('rule_disk_root_usage_high', '根分区使用率过高', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 85}], "tags": [{"mount_point": "/"}]}'::jsonb,
 '1m', '严重', '根分区使用率超过85%', '节点 {{host_ip}} 的根分区（/）使用率为 {{value}}%，超过严重告警阈值85%，系统分区空间不足。', '硬件状态', true),

('rule_disk_data_usage_high', '数据分区使用率过高', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 90}], "tags": [{"mount_point": "/data"}]}'::jsonb,
 '2m', '警告', '数据分区使用率超过90%', '节点 {{host_ip}} 的数据分区（/data）使用率为 {{value}}%，超过警告阈值90%。', '硬件状态', true),

('rule_disk_tmp_usage_high', '临时分区使用率过高', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 80}], "tags": [{"mount_point": "/tmp"}]}'::jsonb,
 '3m', '一般', '临时分区使用率超过80%', '节点 {{host_ip}} 的临时分区（/tmp）使用率为 {{value}}%，超过一般告警阈值80%。', '硬件状态', true),

-- ============================================================================
-- 网络相关告警规则（15个）
-- ============================================================================

('rule_network_rx_rate_high', '网络接收速率过高', 
 '{"stable": "network", "metric": "rx_rate", "conditions": [{"operator": ">", "threshold": 1000000}], "tags": []}'::jsonb,
 '1m', '警告', '网络接收速率超过1GB/s', '节点 {{host_ip}} 的网络接口 {{interface}} 接收速率为 {{value}} KB/s，超过警告阈值1GB/s。', '硬件状态', true),

('rule_network_tx_rate_high', '网络发送速率过高', 
 '{"stable": "network", "metric": "tx_rate", "conditions": [{"operator": ">", "threshold": 1000000}], "tags": []}'::jsonb,
 '1m', '警告', '网络发送速率超过1GB/s', '节点 {{host_ip}} 的网络接口 {{interface}} 发送速率为 {{value}} KB/s，超过警告阈值1GB/s。', '硬件状态', true),

('rule_network_rx_rate_low', '网络接收速率异常低', 
 '{"stable": "network", "metric": "rx_rate", "conditions": [{"operator": "<", "threshold": 1}], "tags": []}'::jsonb,
 '5m', '一般', '网络接收速率低于1KB/s', '节点 {{host_ip}} 的网络接口 {{interface}} 接收速率为 {{value}} KB/s，异常偏低，可能存在网络故障。', '硬件状态', true),

('rule_network_tx_rate_low', '网络发送速率异常低', 
 '{"stable": "network", "metric": "tx_rate", "conditions": [{"operator": "<", "threshold": 1}], "tags": []}'::jsonb,
 '5m', '一般', '网络发送速率低于1KB/s', '节点 {{host_ip}} 的网络接口 {{interface}} 发送速率为 {{value}} KB/s，异常偏低，可能存在网络故障。', '硬件状态', true),

('rule_network_rx_errors', '网络接收错误', 
 '{"stable": "network", "metric": "rx_errors", "conditions": [{"operator": ">", "threshold": 0}], "tags": []}'::jsonb,
 '30s', '严重', '网络接收出现错误', '节点 {{host_ip}} 的网络接口 {{interface}} 接收错误数为 {{value}}，存在网络故障。', '硬件状态', true),

('rule_network_tx_errors', '网络发送错误', 
 '{"stable": "network", "metric": "tx_errors", "conditions": [{"operator": ">", "threshold": 0}], "tags": []}'::jsonb,
 '30s', '严重', '网络发送出现错误', '节点 {{host_ip}} 的网络接口 {{interface}} 发送错误数为 {{value}}，存在网络故障。', '硬件状态', true),

('rule_network_rx_drop_rate_high', '网络接收丢包率过高', 
 '{"stable": "network", "metric": "rx_drop_rate", "conditions": [{"operator": ">", "threshold": 0.01}], "tags": []}'::jsonb,
 '1m', '警告', '网络接收丢包率超过1%', '节点 {{host_ip}} 的网络接口 {{interface}} 接收丢包率为 {{value}}，超过警告阈值1%。', '硬件状态', true),

('rule_network_tx_drop_rate_high', '网络发送丢包率过高', 
 '{"stable": "network", "metric": "tx_drop_rate", "conditions": [{"operator": ">", "threshold": 0.01}], "tags": []}'::jsonb,
 '1m', '警告', '网络发送丢包率超过1%', '节点 {{host_ip}} 的网络接口 {{interface}} 发送丢包率为 {{value}}，超过警告阈值1%。', '硬件状态', true),

('rule_network_rx_bytes_high', '网络接收字节数异常高', 
 '{"stable": "network", "metric": "rx_bytes", "conditions": [{"operator": ">", "threshold": 1099511627776}], "tags": []}'::jsonb,
 '1m', '一般', '网络接收字节数超过1TB', '节点 {{host_ip}} 的网络接口 {{interface}} 累计接收字节数为 {{value}}，超过一般告警阈值。', '硬件状态', true),

('rule_network_tx_bytes_high', '网络发送字节数异常高', 
 '{"stable": "network", "metric": "tx_bytes", "conditions": [{"operator": ">", "threshold": 1099511627776}], "tags": []}'::jsonb,
 '1m', '一般', '网络发送字节数超过1TB', '节点 {{host_ip}} 的网络接口 {{interface}} 累计发送字节数为 {{value}}，超过一般告警阈值。', '硬件状态', true),

-- 针对特定网络接口的告警
('rule_network_eth0_rx_rate_high', 'eth0接口接收速率过高', 
 '{"stable": "network", "metric": "rx_rate", "conditions": [{"operator": ">", "threshold": 500000}], "tags": [{"interface": "eth0"}]}'::jsonb,
 '1m', '警告', 'eth0接口接收速率超过500MB/s', '节点 {{host_ip}} 的eth0接口接收速率为 {{value}} KB/s，超过警告阈值。', '硬件状态', true),

-- ============================================================================
-- GPU 相关告警规则（15个）
-- ============================================================================

('rule_gpu_temperature_critical', 'GPU温度严重过高', 
 '{"stable": "gpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 85}], "tags": []}'::jsonb,
 '30s', '严重', 'GPU温度超过85°C', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）温度为 {{value}}°C，超过严重告警阈值85°C，存在过热风险。', '硬件状态', true),

('rule_gpu_temperature_high', 'GPU温度过高', 
 '{"stable": "gpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 75}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU温度超过75°C', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）温度为 {{value}}°C，超过警告阈值75°C。', '硬件状态', true),

('rule_gpu_temperature_low', 'GPU温度异常低', 
 '{"stable": "gpu", "metric": "temperature", "conditions": [{"operator": "<", "threshold": 20}], "tags": []}'::jsonb,
 '2m', '一般', 'GPU温度低于20°C', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）温度为 {{value}}°C，异常偏低。', '硬件状态', true),


('rule_gpu_mem_usage_high', 'GPU内存使用率过高', 
 '{"stable": "gpu", "metric": "mem_usage", "conditions": [{"operator": ">", "threshold": 0.95}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU内存使用率超过95%', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）内存使用率为 {{value}}，超过警告阈值95%。', '硬件状态', true),

('rule_gpu_mem_usage_low', 'GPU内存使用率异常低', 
 '{"stable": "gpu", "metric": "mem_usage", "conditions": [{"operator": "<", "threshold": 0.05}], "tags": []}'::jsonb,
 '5m', '一般', 'GPU内存使用率低于5%', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）内存使用率为 {{value}}，异常偏低。', '硬件状态', true),

('rule_gpu_mem_used_high', 'GPU内存使用量过高', 
 '{"stable": "gpu", "metric": "mem_used", "conditions": [{"operator": ">", "threshold": 16106127360}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU内存使用量超过15GB', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）内存使用量为 {{value}} 字节，超过警告阈值。', '硬件状态', true),

('rule_gpu_power_high', 'GPU功耗过高', 
 '{"stable": "gpu", "metric": "power", "conditions": [{"operator": ">", "threshold": 250}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU功耗超过250W', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）功耗为 {{value}}W，超过警告阈值250W。', '硬件状态', true),

('rule_gpu_power_low', 'GPU功耗异常低', 
 '{"stable": "gpu", "metric": "power", "conditions": [{"operator": "<", "threshold": 10}], "tags": []}'::jsonb,
 '2m', '一般', 'GPU功耗低于10W', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）功耗为 {{value}}W，异常偏低。', '硬件状态', true),

('rule_gpu_compute_usage_high', 'GPU计算使用率过高', 
 '{"stable": "gpu", "metric": "compute_usage", "conditions": [{"operator": ">", "threshold": 95}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU计算使用率超过95%', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）计算使用率为 {{value}}%，超过警告阈值95%。', '硬件状态', true),

('rule_gpu_compute_usage_low', 'GPU计算使用率异常低', 
 '{"stable": "gpu", "metric": "compute_usage", "conditions": [{"operator": "<", "threshold": 5}], "tags": []}'::jsonb,
 '5m', '一般', 'GPU计算使用率低于5%', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）计算使用率为 {{value}}%，异常偏低。', '硬件状态', true),

-- ============================================================================
-- 节点存活相关告警规则（5个）
-- ============================================================================

('rule_node_alive_false', '节点离线', 
 '{"stable": "alive", "metric": "alive", "conditions": [{"operator": "==", "threshold": 0}], "tags": []}'::jsonb,
 '10s', '严重', '节点已离线', '节点 {{host_ip}} 已离线，无法获取心跳信息，请检查节点状态和网络连接。', '系统告警', true),

('rule_node_alive_recovered', '节点恢复在线', 
 '{"stable": "alive", "metric": "alive", "conditions": [{"operator": "==", "threshold": 1}], "tags": []}'::jsonb,
 '5s', '一般', '节点恢复在线', '节点 {{host_ip}} 已恢复在线，系统正常运行。', '系统告警', false),

-- ============================================================================
-- 复合条件告警规则（10个）
-- ============================================================================

('rule_cpu_memory_high', 'CPU和内存同时过高', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 80}, {"operator": "<", "threshold": 100}], "tags": []}'::jsonb,
 '2m', '严重', 'CPU和内存使用率同时超过80%', '节点 {{host_ip}} 的CPU和内存使用率同时超过80%，系统负载严重。', '硬件状态', true),

('rule_disk_network_high', '磁盘和网络同时高负载', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 85}], "tags": []}'::jsonb,
 '3m', '警告', '磁盘使用率和网络负载同时过高', '节点 {{host_ip}} 的磁盘使用率超过85%且网络负载较高，可能存在I/O瓶颈。', '硬件状态', true),

('rule_gpu_temp_usage_high', 'GPU温度和使用率同时过高', 
 '{"stable": "gpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 80}], "tags": []}'::jsonb,
 '1m', '严重', 'GPU温度和使用率同时过高', '节点 {{host_ip}} 的GPU温度超过80°C且使用率较高，存在过热风险。', '硬件状态', true),

-- ============================================================================
-- 补充告警规则（达到100个）
-- ============================================================================

('rule_cpu_voltage_abnormal', 'CPU电压异常', 
 '{"stable": "cpu", "metric": "voltage", "conditions": [{"operator": "<", "threshold": 0.8}, {"operator": ">", "threshold": 1.2}], "tags": []}'::jsonb,
 '30s', '严重', 'CPU电压超出正常范围', '节点 {{host_ip}} 的CPU电压为 {{value}}V，超出正常范围（0.8V-1.2V）。', '硬件状态', true),

('rule_memory_total_low', '内存总量不足', 
 '{"stable": "memory", "metric": "total", "conditions": [{"operator": "<", "threshold": 8589934592}], "tags": []}'::jsonb,
 '1m', '一般', '内存总量低于8GB', '节点 {{host_ip}} 的内存总量为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值8GB。', '硬件状态', true),

('rule_disk_total_low', '磁盘总量不足', 
 '{"stable": "disk", "metric": "total", "conditions": [{"operator": "<", "threshold": 107374182400}], "tags": []}'::jsonb,
 '1m', '一般', '磁盘总量低于100GB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）总量为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值100GB。', '硬件状态', true),

('rule_network_state_down', '网络接口状态异常', 
 '{"stable": "network", "metric": "state", "conditions": [{"operator": "!=", "threshold": 0}], "tags": []}'::jsonb,
 '10s', '严重', '网络接口状态非正常', '节点 {{host_ip}} 的网络接口 {{interface}} 状态为 {{value}}，非正常状态（0表示正常）。', '硬件状态', true),

('rule_gpu_allocated_full', 'GPU全部已分配', 
 '{"stable": "gpu", "metric": "gpu_allocated", "conditions": [{"operator": ">=", "threshold": 1}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU全部已分配', '节点 {{host_ip}} 的GPU已分配数量为 {{value}}，所有GPU均被占用。', '硬件状态', true),

-- ============================================================================
-- 补充告警规则（继续补充到100个）
-- ============================================================================

('rule_cpu_usage_high_4', 'CPU使用率过高-临界', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 95}], "tags": []}'::jsonb,
 '20s', '严重', 'CPU使用率超过95%', '节点 {{host_ip}} 的CPU使用率已达到 {{value}}%，超过临界告警阈值95%，系统即将过载。', '硬件状态', true),

('rule_cpu_usage_high_5', 'CPU使用率过高-中等', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 60}], "tags": []}'::jsonb,
 '3m', '一般', 'CPU使用率超过60%', '节点 {{host_ip}} 的CPU使用率为 {{value}}%，超过一般告警阈值60%。', '硬件状态', true),

('rule_cpu_temp_warning', 'CPU温度警告', 
 '{"stable": "cpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 70}], "tags": []}'::jsonb,
 '2m', '警告', 'CPU温度超过70°C', '节点 {{host_ip}} 的CPU温度为 {{value}}°C，超过警告阈值70°C。', '硬件状态', true),

('rule_cpu_power_warning', 'CPU功耗警告', 
 '{"stable": "cpu", "metric": "power", "conditions": [{"operator": ">", "threshold": 80}], "tags": []}'::jsonb,
 '2m', '警告', 'CPU功耗超过80W', '节点 {{host_ip}} 的CPU功耗为 {{value}}W，超过警告阈值80W。', '硬件状态', true),

('rule_memory_usage_high_4', '内存使用率过高-临界', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 85}], "tags": []}'::jsonb,
 '1m', '严重', '内存使用率超过85%', '节点 {{host_ip}} 的内存使用率为 {{value}}%，超过严重告警阈值85%。', '硬件状态', true),

('rule_memory_usage_high_5', '内存使用率过高-中等', 
 '{"stable": "memory", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 60}], "tags": []}'::jsonb,
 '3m', '一般', '内存使用率超过60%', '节点 {{host_ip}} 的内存使用率为 {{value}}%，超过一般告警阈值60%。', '硬件状态', true),

('rule_memory_free_low_3', '内存空闲量偏低', 
 '{"stable": "memory", "metric": "free", "conditions": [{"operator": "<", "threshold": 4294967296}], "tags": []}'::jsonb,
 '2m', '一般', '内存空闲量低于4GB', '节点 {{host_ip}} 的内存空闲量为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值4GB。', '硬件状态', true),

('rule_disk_usage_high_4', '磁盘使用率过高-临界', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 88}], "tags": []}'::jsonb,
 '1m', '严重', '磁盘使用率超过88%', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用率为 {{value}}%，超过严重告警阈值88%。', '硬件状态', true),

('rule_disk_usage_high_5', '磁盘使用率过高-中等', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 75}], "tags": []}'::jsonb,
 '3m', '一般', '磁盘使用率超过75%', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）使用率为 {{value}}%，超过一般告警阈值75%。', '硬件状态', true),

('rule_disk_free_low_4', '磁盘空闲空间不足-100GB', 
 '{"stable": "disk", "metric": "free", "conditions": [{"operator": "<", "threshold": 107374182400}], "tags": []}'::jsonb,
 '2m', '一般', '磁盘空闲空间低于100GB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）空闲空间为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值100GB。', '硬件状态', true),

('rule_disk_var_usage_high', 'var分区使用率过高', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 85}], "tags": [{"mount_point": "/var"}]}'::jsonb,
 '2m', '警告', 'var分区使用率超过85%', '节点 {{host_ip}} 的var分区（/var）使用率为 {{value}}%，超过警告阈值85%。', '硬件状态', true),

('rule_disk_home_usage_high', 'home分区使用率过高', 
 '{"stable": "disk", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 90}], "tags": [{"mount_point": "/home"}]}'::jsonb,
 '2m', '警告', 'home分区使用率超过90%', '节点 {{host_ip}} 的home分区（/home）使用率为 {{value}}%，超过警告阈值90%。', '硬件状态', true),

('rule_network_rx_rate_medium', '网络接收速率中等偏高', 
 '{"stable": "network", "metric": "rx_rate", "conditions": [{"operator": ">", "threshold": 500000}], "tags": []}'::jsonb,
 '2m', '一般', '网络接收速率超过500MB/s', '节点 {{host_ip}} 的网络接口 {{interface}} 接收速率为 {{value}} KB/s，超过一般告警阈值500MB/s。', '硬件状态', true),

('rule_network_tx_rate_medium', '网络发送速率中等偏高', 
 '{"stable": "network", "metric": "tx_rate", "conditions": [{"operator": ">", "threshold": 500000}], "tags": []}'::jsonb,
 '2m', '一般', '网络发送速率超过500MB/s', '节点 {{host_ip}} 的网络接口 {{interface}} 发送速率为 {{value}} KB/s，超过一般告警阈值500MB/s。', '硬件状态', true),

('rule_network_eth1_rx_rate_high', 'eth1接口接收速率过高', 
 '{"stable": "network", "metric": "rx_rate", "conditions": [{"operator": ">", "threshold": 500000}], "tags": [{"interface": "eth1"}]}'::jsonb,
 '1m', '警告', 'eth1接口接收速率超过500MB/s', '节点 {{host_ip}} 的eth1接口接收速率为 {{value}} KB/s，超过警告阈值。', '硬件状态', true),

('rule_network_docker0_rx_rate_high', 'docker0接口接收速率过高', 
 '{"stable": "network", "metric": "rx_rate", "conditions": [{"operator": ">", "threshold": 100000}], "tags": [{"interface": "docker0"}]}'::jsonb,
 '1m', '一般', 'docker0接口接收速率超过100MB/s', '节点 {{host_ip}} 的docker0接口接收速率为 {{value}} KB/s，超过一般告警阈值。', '硬件状态', true),

('rule_gpu_temperature_warning', 'GPU温度警告', 
 '{"stable": "gpu", "metric": "temperature", "conditions": [{"operator": ">", "threshold": 70}], "tags": []}'::jsonb,
 '2m', '警告', 'GPU温度超过70°C', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）温度为 {{value}}°C，超过警告阈值70°C。', '硬件状态', true),


('rule_gpu_mem_usage_medium', 'GPU内存使用率中等偏高', 
 '{"stable": "gpu", "metric": "mem_usage", "conditions": [{"operator": ">", "threshold": 0.85}], "tags": []}'::jsonb,
 '2m', '一般', 'GPU内存使用率超过85%', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）内存使用率为 {{value}}，超过一般告警阈值85%。', '硬件状态', true),

('rule_gpu_power_medium', 'GPU功耗中等偏高', 
 '{"stable": "gpu", "metric": "power", "conditions": [{"operator": ">", "threshold": 200}], "tags": []}'::jsonb,
 '2m', '一般', 'GPU功耗超过200W', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）功耗为 {{value}}W，超过一般告警阈值200W。', '硬件状态', true),

('rule_gpu_compute_usage_medium', 'GPU计算使用率中等偏高', 
 '{"stable": "gpu", "metric": "compute_usage", "conditions": [{"operator": ">", "threshold": 80}], "tags": []}'::jsonb,
 '2m', '一般', 'GPU计算使用率超过80%', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）计算使用率为 {{value}}%，超过一般告警阈值80%。', '硬件状态', true),

('rule_node_alive_timeout', '节点心跳超时', 
 '{"stable": "alive", "metric": "alive", "conditions": [{"operator": "==", "threshold": 0}], "tags": []}'::jsonb,
 '30s', '严重', '节点心跳超时', '节点 {{host_ip}} 心跳超时，超过30秒未收到心跳信息，节点可能已离线。', '系统告警', true),

('rule_node_alive_intermittent', '节点心跳不稳定', 
 '{"stable": "alive", "metric": "alive", "conditions": [{"operator": "==", "threshold": 0}], "tags": []}'::jsonb,
 '5s', '警告', '节点心跳不稳定', '节点 {{host_ip}} 心跳不稳定，可能存在网络波动。', '系统告警', false),

('rule_cpu_load_1m_medium', 'CPU 1分钟负载中等偏高', 
 '{"stable": "cpu", "metric": "load_avg_1m", "conditions": [{"operator": ">", "threshold": 5}], "tags": []}'::jsonb,
 '2m', '一般', 'CPU 1分钟平均负载超过5', '节点 {{host_ip}} 的CPU 1分钟平均负载为 {{value}}，超过一般告警阈值5。', '硬件状态', true),

('rule_cpu_load_5m_medium', 'CPU 5分钟负载中等偏高', 
 '{"stable": "cpu", "metric": "load_avg_5m", "conditions": [{"operator": ">", "threshold": 4}], "tags": []}'::jsonb,
 '3m', '一般', 'CPU 5分钟平均负载超过4', '节点 {{host_ip}} 的CPU 5分钟平均负载为 {{value}}，超过一般告警阈值4。', '硬件状态', true),

('rule_cpu_load_15m_medium', 'CPU 15分钟负载中等偏高', 
 '{"stable": "cpu", "metric": "load_avg_15m", "conditions": [{"operator": ">", "threshold": 3}], "tags": []}'::jsonb,
 '3m', '一般', 'CPU 15分钟平均负载超过3', '节点 {{host_ip}} 的CPU 15分钟平均负载为 {{value}}，超过一般告警阈值3。', '硬件状态', true),

('rule_memory_total_medium', '内存总量中等偏低', 
 '{"stable": "memory", "metric": "total", "conditions": [{"operator": "<", "threshold": 17179869184}], "tags": []}'::jsonb,
 '1m', '一般', '内存总量低于16GB', '节点 {{host_ip}} 的内存总量为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值16GB。', '硬件状态', true),

('rule_disk_total_medium', '磁盘总量中等偏低', 
 '{"stable": "disk", "metric": "total", "conditions": [{"operator": "<", "threshold": 214748364800}], "tags": []}'::jsonb,
 '1m', '一般', '磁盘总量低于200GB', '节点 {{host_ip}} 的磁盘（挂载点：{{mount_point}}）总量为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值200GB。', '硬件状态', true),

('rule_network_rx_packets_high', '网络接收包数异常高', 
 '{"stable": "network", "metric": "rx_packets", "conditions": [{"operator": ">", "threshold": 1000000000}], "tags": []}'::jsonb,
 '1m', '一般', '网络接收包数超过10亿', '节点 {{host_ip}} 的网络接口 {{interface}} 累计接收包数为 {{value}}，超过一般告警阈值。', '硬件状态', true),

('rule_network_tx_packets_high', '网络发送包数异常高', 
 '{"stable": "network", "metric": "tx_packets", "conditions": [{"operator": ">", "threshold": 1000000000}], "tags": []}'::jsonb,
 '1m', '一般', '网络发送包数超过10亿', '节点 {{host_ip}} 的网络接口 {{interface}} 累计发送包数为 {{value}}，超过一般告警阈值。', '硬件状态', true),

('rule_gpu_mem_total_low', 'GPU内存总量不足', 
 '{"stable": "gpu", "metric": "mem_total", "conditions": [{"operator": "<", "threshold": 8589934592}], "tags": []}'::jsonb,
 '1m', '一般', 'GPU内存总量低于8GB', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）内存总量为 {{value}} 字节（约 {{value}}GB），低于一般告警阈值8GB。', '硬件状态', true),

('rule_gpu_num_low', 'GPU数量不足', 
 '{"stable": "gpu", "metric": "gpu_num", "conditions": [{"operator": "<", "threshold": 1}], "tags": []}'::jsonb,
 '1m', '警告', 'GPU数量为0', '节点 {{host_ip}} 的GPU数量为 {{value}}，未检测到GPU设备。', '硬件状态', true),

('rule_cpu_core_count_low', 'CPU核心数不足', 
 '{"stable": "cpu", "metric": "core_count", "conditions": [{"operator": "<", "threshold": 4}], "tags": []}'::jsonb,
 '1m', '一般', 'CPU核心数低于4', '节点 {{host_ip}} 的CPU核心数为 {{value}}，低于一般告警阈值4核。', '硬件状态', true),

('rule_cpu_core_allocated_full', 'CPU核心全部已分配', 
 '{"stable": "cpu", "metric": "core_allocated", "conditions": [{"operator": ">=", "threshold": 8}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU核心全部已分配', '节点 {{host_ip}} 的CPU已分配核心数为 {{value}}，所有核心均被分配。', '硬件状态', true),

('rule_system_health_check', '系统健康检查', 
 '{"stable": "cpu", "metric": "usage_percent", "conditions": [{"operator": ">", "threshold": 50}], "tags": []}'::jsonb,
 '5m', '一般', '系统负载检查', '节点 {{host_ip}} 的CPU使用率为 {{value}}%，系统运行正常。', '系统告警', false),

('rule_cpu_current_high', 'CPU电流过高', 
 '{"stable": "cpu", "metric": "current", "conditions": [{"operator": ">", "threshold": 50}], "tags": []}'::jsonb,
 '1m', '警告', 'CPU电流超过50A', '节点 {{host_ip}} 的CPU电流为 {{value}}A，超过警告阈值50A。', '硬件状态', true),

('rule_gpu_free_status_check', 'GPU空闲状态检查', 
 '{"stable": "gpu", "metric": "free", "conditions": [{"operator": "==", "threshold": 0}], "tags": []}'::jsonb,
 '1m', '一般', 'GPU处于占用状态', '节点 {{host_ip}} 的GPU（索引：{{gpu_index}}，名称：{{name}}）当前处于占用状态。', '硬件状态', true),

('rule_network_rx_packets_rate_high', '网络接收包速率过高', 
 '{"stable": "network", "metric": "rx_packets", "conditions": [{"operator": ">", "threshold": 100000000}], "tags": []}'::jsonb,
 '1m', '一般', '网络接收包数超过1亿', '节点 {{host_ip}} 的网络接口 {{interface}} 累计接收包数为 {{value}}，超过一般告警阈值。', '硬件状态', true);

-- 提交事务
COMMIT;

-- 显示插入结果
SELECT COUNT(*) as total_rules, 
       COUNT(*) FILTER (WHERE enabled = true) as enabled_rules,
       COUNT(*) FILTER (WHERE enabled = false) as disabled_rules,
       alert_type,
       COUNT(*) as count_by_type
FROM alert_rule
GROUP BY alert_type
ORDER BY count_by_type DESC;

