-- BMC传感器模拟数据插入脚本
-- 为多个主机和传感器生成最近24小时的数据

-- 生成函数：为指定主机和传感器生成一段时间的时序数据
DO $$
DECLARE
    host_ip TEXT;
    sensor_time TIMESTAMPTZ;
    time_offset INTEGER;
    base_value INTEGER;
    sensor_val_H SMALLINT;
    sensor_val_L SMALLINT;
    i INTEGER;
BEGIN
    -- 为每个主机生成数据
    FOR host_ip IN 
        SELECT unnest(ARRAY['192.168.0.5', '192.168.0.37', '192.168.0.69', '192.168.1.5', '192.168.1.37'])
    LOOP
        -- 温度传感器（3个）
        FOR i IN 1..3 LOOP
            FOR time_offset IN 0..143 LOOP
                sensor_time := NOW() - INTERVAL '24 hours' + (time_offset * INTERVAL '10 minutes');
                
                -- 生成温度值：20-80度，带波动
                base_value := 40 + (RANDOM() * 40)::INTEGER + 
                              (SIN(EXTRACT(EPOCH FROM sensor_time) / 3600.0) * 5)::INTEGER;
                sensor_val_H := LEAST(base_value, 255)::SMALLINT;
                sensor_val_L := (RANDOM() * 100)::SMALLINT;
                
                INSERT INTO bmc_sensor ("time", host_ip, sensorseq, sensortype, sensorname, 
                                       sensorvalue_L, sensorvalue_H, sensoralmtype)
                VALUES (
                    sensor_time,
                    host_ip::inet,
                    i::SMALLINT,
                    1::SMALLINT,  -- 温度传感器类型
                    'TEMP' || i::TEXT,
                    sensor_val_L,
                    sensor_val_H,
                    CASE WHEN base_value > 70 THEN 1::SMALLINT ELSE 0::SMALLINT END
                );
            END LOOP;
        END LOOP;
        
        -- 电压传感器（2个）
        FOR i IN 1..2 LOOP
            FOR time_offset IN 0..143 LOOP
                sensor_time := NOW() - INTERVAL '24 hours' + (time_offset * INTERVAL '10 minutes');
                
                -- 生成电压值：12-15V（以0.01V为单位，所以120-150）
                base_value := 120 + (RANDOM() * 30)::INTEGER + 
                              (SIN(EXTRACT(EPOCH FROM sensor_time) / 1800.0) * 2)::INTEGER;
                sensor_val_H := LEAST(base_value, 255)::SMALLINT;
                sensor_val_L := (RANDOM() * 100)::SMALLINT;
                
                INSERT INTO bmc_sensor ("time", host_ip, sensorseq, sensortype, sensorname, 
                                       sensorvalue_L, sensorvalue_H, sensoralmtype)
                VALUES (
                    sensor_time,
                    host_ip::inet,
                    (10 + i)::SMALLINT,
                    2::SMALLINT,  -- 电压传感器类型
                    'VOLT' || i::TEXT,
                    sensor_val_L,
                    sensor_val_H,
                    0::SMALLINT
                );
            END LOOP;
        END LOOP;
        
        -- 风扇转速传感器（2个）
        FOR i IN 1..2 LOOP
            FOR time_offset IN 0..143 LOOP
                sensor_time := NOW() - INTERVAL '24 hours' + (time_offset * INTERVAL '10 minutes');
                
                -- 生成风扇转速：2000-5000 RPM
                base_value := 3000 + (RANDOM() * 2000)::INTEGER + 
                              (SIN(EXTRACT(EPOCH FROM sensor_time) / 900.0) * 200)::INTEGER;
                sensor_val_H := LEAST(base_value, 255)::SMALLINT;
                sensor_val_L := (RANDOM() * 100)::SMALLINT;
                
                INSERT INTO bmc_sensor ("time", host_ip, sensorseq, sensortype, sensorname, 
                                       sensorvalue_L, sensorvalue_H, sensoralmtype)
                VALUES (
                    sensor_time,
                    host_ip::inet,
                    (20 + i)::SMALLINT,
                    3::SMALLINT,  -- 风扇传感器类型
                    'FAN' || i::TEXT,
                    sensor_val_L,
                    sensor_val_H,
                    0::SMALLINT
                );
            END LOOP;
        END LOOP;
        
        -- 电流传感器（2个）
        FOR i IN 1..2 LOOP
            FOR time_offset IN 0..143 LOOP
                sensor_time := NOW() - INTERVAL '24 hours' + (time_offset * INTERVAL '10 minutes');
                
                -- 生成电流值：5-15A
                base_value := 10 + (RANDOM() * 10)::INTEGER + 
                              (SIN(EXTRACT(EPOCH FROM sensor_time) / 1200.0) * 2)::INTEGER;
                sensor_val_H := LEAST(base_value, 255)::SMALLINT;
                sensor_val_L := (RANDOM() * 100)::SMALLINT;
                
                INSERT INTO bmc_sensor ("time", host_ip, sensorseq, sensortype, sensorname, 
                                       sensorvalue_L, sensorvalue_H, sensoralmtype)
                VALUES (
                    sensor_time,
                    host_ip::inet,
                    (30 + i)::SMALLINT,
                    4::SMALLINT,  -- 电流传感器类型
                    'CURR' || i::TEXT,
                    sensor_val_L,
                    sensor_val_H,
                    CASE WHEN base_value > 13 THEN 1::SMALLINT ELSE 0::SMALLINT END
                );
            END LOOP;
        END LOOP;
        
        -- 功率传感器（1个）
        FOR time_offset IN 0..143 LOOP
            sensor_time := NOW() - INTERVAL '24 hours' + (time_offset * INTERVAL '10 minutes');
            
            -- 生成功率值：100-300W
            base_value := 200 + (RANDOM() * 200)::INTEGER + 
                          (SIN(EXTRACT(EPOCH FROM sensor_time) / 1100.0) * 20)::INTEGER;
            sensor_val_H := LEAST(base_value, 255)::SMALLINT;
            sensor_val_L := (RANDOM() * 100)::SMALLINT;
            
            INSERT INTO bmc_sensor ("time", host_ip, sensorseq, sensortype, sensorname, 
                                   sensorvalue_L, sensorvalue_H, sensoralmtype)
            VALUES (
                sensor_time,
                host_ip::inet,
                40::SMALLINT,
                5::SMALLINT,  -- 功率传感器类型
                'PWR1',
                sensor_val_L,
                sensor_val_H,
                0::SMALLINT
            );
        END LOOP;
        
        RAISE NOTICE '已为主机 % 插入传感器数据', host_ip;
    END LOOP;
    
    RAISE NOTICE '模拟数据插入完成！';
END $$;

-- 统计插入的数据
SELECT 
    host_ip,
    COUNT(DISTINCT sensorname) as sensor_count,
    COUNT(*) as total_records,
    MIN("time") as earliest_time,
    MAX("time") as latest_time
FROM bmc_sensor
GROUP BY host_ip
ORDER BY host_ip;

