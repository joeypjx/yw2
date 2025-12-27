# 测试脚本使用说明

## 环境设置

### 1. 创建虚拟环境（首次使用）

```bash
cd tests
python3 -m venv venv
```

### 2. 激活虚拟环境

**macOS/Linux:**
```bash
source venv/bin/activate
```

**Windows:**
```bash
venv\Scripts\activate
```

### 3. 安装依赖

```bash
pip install -r requirements.txt
```

## 使用方法

### 激活虚拟环境后运行

```bash
# 使用默认配置（localhost:18888，每 5 秒发送一次）
python send_test_data.py

# 自定义服务器地址和端口
python send_test_data.py localhost 18888

# 自定义服务器地址、端口和发送间隔（秒）
python send_test_data.py localhost 18888 5
```

### 退出虚拟环境

```bash
deactivate
```

## 功能说明

### send_test_data.py
- 每隔指定时间（默认 5 秒）调用 `/heartbeat` 和 `/resource` 接口
- 自动从 `docs/node/node_api.md` 和 `docs/monitor/monitor_api.md` 提取测试数据
- **支持模拟多节点**：默认模拟 1-9 号机箱，每个机箱 1-12 槽位的板卡数据
- 动态随机化资源指标数值（CPU、内存、磁盘、GPU、网络等）
- 为每个节点生成唯一的 host_ip、序列号等标识信息
- 显示请求状态和响应信息
- 支持 Ctrl+C 优雅停止

**使用方法：**
```bash
# 使用默认配置（1-9号机箱，1-12槽位，localhost:18888，每 5 秒发送）
python send_test_data.py

# 自定义服务器地址和端口
python send_test_data.py localhost 18888

# 自定义服务器地址、端口和发送间隔
python send_test_data.py localhost 18888 5

# 自定义机箱和槽位范围（服务器地址、端口、间隔、起始机箱、结束机箱、起始槽位、结束槽位）
python send_test_data.py localhost 18888 5 1 9 1 12

# 示例：只模拟 1-3 号机箱，每个机箱 1-6 槽位
python send_test_data.py localhost 18888 5 1 3 1 6
```

**节点数据生成规则：**
- **IP地址计算规则**（对应 `src/utils/ip_address_utils.cpp`）：
  - 格式：`192.168.{network_id}.{host_id}`
  - **network_id 计算**：
    - slot_id 1-7: `network_id = box_id * 2`
    - slot_id 8-14: `network_id = box_id * 2 + 1`
  - **host_id 映射**（根据 slot_id）：
    - slot 1 → host_id 5
    - slot 2 → host_id 37
    - slot 3 → host_id 69
    - slot 4 → host_id 101
    - slot 5 → host_id 133
    - slot 6 → host_id 170
    - slot 7 → host_id 180
    - slot 8 → host_id 5
    - slot 9 → host_id 37
    - slot 10 → host_id 69
    - slot 11 → host_id 101
    - slot 12 → host_id 133
    - slot 13 → host_id 181
    - slot 14 → host_id 182
  - **示例**：
    - box1-slot1 → 192.168.2.5 (network_id=2, host_id=5)
    - box1-slot8 → 192.168.3.5 (network_id=3, host_id=5)
    - box5-slot4 → 192.168.10.101 (network_id=10, host_id=101)
    - box5-slot12 → 192.168.11.133 (network_id=11, host_id=133)
- hostname 格式：`box{box_id}-slot{slot_id}.localdomain`
- serial_number：根据 box_id 和 slot_id 生成唯一序列号
- **槽位范围**：支持 1-14（默认 1-12）

### send_bmc_multicast.py
- 发送 BMC UDP 组播报文（UdpInfo 格式）
- 按照 `include/bmc/bmc_model.h` 中定义的结构构造报文
- 使用 `config/config.json` 中的组播配置（默认：224.100.200.15:5715）
- 自动生成测试数据（风扇、传感器、电源板、负载槽板等）
- 支持自定义 box_id、组播地址、端口和发送间隔
- 序列号自动递增（1-65535 循环）

**使用方法：**
```bash
# 使用默认配置（box_id=1, 224.100.200.15:5715, 每 5 秒发送）
python send_bmc_multicast.py

# 自定义 box_id
python send_bmc_multicast.py 2

# 自定义 box_id、组播地址和端口
python send_bmc_multicast.py 2 224.100.200.15 5715

# 自定义所有参数（box_id, 组播地址, 端口, 发送间隔）
python send_bmc_multicast.py 2 224.100.200.15 5715 10
```

