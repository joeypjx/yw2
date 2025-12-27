#!/usr/bin/env python3
"""
测试脚本：每隔 5 秒调用 /heartbeat 和 /resource 接口
发送 docs/node/node_api.md 和 docs/monitor/monitor_api.md 中的测试数据
支持模拟 1-9 号机箱，每个机箱 1-12 槽位的板卡数据
"""

import json
import time
import sys
import os
import copy
import random
import requests
from pathlib import Path
from typing import Dict, Any, Optional, List, Tuple

# 默认配置
DEFAULT_HOST = "localhost"
DEFAULT_PORT = 18888
DEFAULT_INTERVAL = 5  # 秒
DEFAULT_BOX_START = 1  # 起始机箱号
DEFAULT_BOX_END = 9    # 结束机箱号
DEFAULT_SLOT_START = 1  # 起始槽位号
DEFAULT_SLOT_END = 12   # 结束槽位号（支持1-14，但默认1-12）

# 获取项目根目录
PROJECT_ROOT = Path(__file__).parent.parent
HEARTBEAT_DOC = PROJECT_ROOT / "docs" / "node" / "node_api.md"
RESOURCE_DOC = PROJECT_ROOT / "docs" / "monitor" / "monitor_api.md"


def extract_json_from_doc(doc_path: Path) -> Optional[Dict[str, Any]]:
    """
    从文档文件中提取 JSON 数据
    文档格式：第一行可能有一个 JSON，实际请求数据从"终端发来的请求格式"之后开始
    """
    try:
        with open(doc_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        # 查找"终端发来的请求格式"这一行，实际请求数据在它之后
        request_data_start = None
        for i, line in enumerate(lines):
            if "终端发来的请求格式" in line or "请求格式" in line:
                request_data_start = i + 1
                break
        
        # 如果没有找到标记行，从第 5 行开始查找（兼容处理）
        if request_data_start is None:
            request_data_start = 4  # 第 5 行（索引从 0 开始）
        
        # 从指定位置开始查找第一个完整的 JSON 对象
        brace_count = 0
        json_lines = []
        
        for i in range(request_data_start, len(lines)):
            line = lines[i]
            if '{' in line or json_lines:  # 开始收集或继续收集
                json_lines.append(line)
                brace_count += line.count('{') - line.count('}')
                if brace_count == 0 and json_lines:
                    # 找到了完整的 JSON
                    json_str = ''.join(json_lines)
                    try:
                        return json.loads(json_str)
                    except json.JSONDecodeError as e:
                        print(f"警告：解析 JSON 失败: {e}")
                        print(f"JSON 内容: {json_str[:200]}...")
                        return None
        
        print(f"错误：在 {doc_path} 中未找到有效的 JSON 数据")
        return None
        
    except FileNotFoundError:
        print(f"错误：文件不存在: {doc_path}")
        return None
    except Exception as e:
        print(f"错误：读取文件失败 {doc_path}: {e}")
        return None


def get_host_id_by_slot_id(slot_id: int) -> int:
    """
    根据 slot_id 获取 host_id（对应 ip_address_utils.cpp 中的规则）
    """
    slot_to_host_id = {
        1: 5,
        2: 37,
        3: 69,
        4: 101,
        5: 133,
        6: 170,
        7: 180,
        8: 5,
        9: 37,
        10: 69,
        11: 101,
        12: 133,
        13: 181,
        14: 182
    }
    return slot_to_host_id.get(slot_id, -1)


def calculate_host_ip(box_id: int, slot_id: int) -> str:
    """
    根据 box_id 和 slot_id 计算 host_ip（对应 ip_address_utils.cpp 中的规则）
    规则：
    - slot_id 1-7: network_id = box_id * 2
    - slot_id 8-14: network_id = box_id * 2 + 1
    - host_id 根据 slot_id 映射（见 get_host_id_by_slot_id）
    - IP格式：192.168.{network_id}.{host_id}
    """
    # 验证参数范围
    if box_id < 1 or box_id > 9:
        return ""
    
    if slot_id < 1 or slot_id > 14:
        return ""
    
    # 获取 host_id
    host_id = get_host_id_by_slot_id(slot_id)
    if host_id == -1:
        return ""
    
    # 计算 network_id
    if slot_id >= 1 and slot_id <= 7:
        network_id = box_id * 2
    elif slot_id >= 8 and slot_id <= 14:
        network_id = box_id * 2 + 1
    else:
        return ""
    
    # 构建IP地址
    return f"192.168.{network_id}.{host_id}"


def generate_node_data(
    base_heartbeat: Dict[str, Any],
    base_resource: Dict[str, Any],
    box_id: int,
    slot_id: int
) -> Tuple[Dict[str, Any], Dict[str, Any]]:
    """
    根据 box_id 和 slot_id 生成节点数据
    返回 (heartbeat_data, resource_data)
    """
    # 深拷贝基础数据
    heartbeat_data = copy.deepcopy(base_heartbeat)
    resource_data = copy.deepcopy(base_resource)
    
    # 使用正确的IP地址计算规则
    host_ip = calculate_host_ip(box_id, slot_id)
    if not host_ip:
        raise ValueError(f"无效的 box_id={box_id} 或 slot_id={slot_id}")
    
    # 修改 heartbeat 数据
    if "data" in heartbeat_data:
        heartbeat_data["data"]["box_id"] = box_id
        heartbeat_data["data"]["slot_id"] = slot_id
        heartbeat_data["data"]["host_ip"] = host_ip
        heartbeat_data["data"]["hostname"] = f"box{box_id}-slot{slot_id}.localdomain"
        # 生成唯一的序列号
        heartbeat_data["data"]["serial_number"] = f"BOX{box_id:02d}SLOT{slot_id:02d}{random.randint(1000000, 9999999)}"
    
    # 修改 resource 数据
    if "data" in resource_data:
        resource_data["data"]["host_ip"] = host_ip
        
        # 修改组件数据中的 host_ip
        if "component" in resource_data["data"]:
            for component in resource_data["data"]["component"]:
                # 可以在这里为不同组件生成不同的数据
                pass
    
    return heartbeat_data, resource_data


def randomize_resource_metrics(data: Dict[str, Any]) -> Dict[str, Any]:
    """
    随机化资源指标数值，使测试数据更真实
    """
    data = copy.deepcopy(data)  # 深拷贝，避免修改原始数据
    
    if "data" not in data:
        return data
    
    resource_data = data["data"]
    
    # 1. 修改主机 CPU 指标
    if "resource" in resource_data and "cpu" in resource_data["resource"]:
        cpu = resource_data["resource"]["cpu"]
        # CPU 使用率：20-80%
        cpu["usage_percent"] = round(random.uniform(20.0, 80.0), 2)
        # 负载平均值：基于 CPU 使用率计算
        base_load = cpu["usage_percent"] / 100.0
        cpu["load_avg_1m"] = round(random.uniform(base_load * 0.8, base_load * 1.2), 2)
        cpu["load_avg_5m"] = round(random.uniform(base_load * 0.9, base_load * 1.1), 2)
        cpu["load_avg_15m"] = round(random.uniform(base_load * 0.95, base_load * 1.05), 2)
        # CPU 温度：40-70 度
        cpu["temperature"] = random.randint(40, 70)
        # CPU 功耗：5-25 瓦
        cpu["power"] = random.randint(5, 25)
    
    # 2. 修改主机内存指标
    if "resource" in resource_data and "memory" in resource_data["resource"]:
        memory = resource_data["resource"]["memory"]
        # 内存使用率：30-70%
        usage_percent = round(random.uniform(30.0, 70.0), 2)
        memory["usage_percent"] = usage_percent
        # 根据使用率计算已用和空闲内存
        if "total" in memory:
            total = memory["total"]
            memory["used"] = int(total * usage_percent / 100)
            memory["free"] = total - memory["used"]
    
    # 3. 修改磁盘使用率
    if "resource" in resource_data and "disk" in resource_data["resource"]:
        for disk in resource_data["resource"]["disk"]:
            # 磁盘使用率：40-80%
            usage_percent = round(random.uniform(40.0, 80.0), 2)
            disk["usage_percent"] = usage_percent
            if "total" in disk:
                total = disk["total"]
                disk["used"] = int(total * usage_percent / 100)
                disk["free"] = total - disk["used"]
    
    # 4. 修改 GPU 指标
    if "resource" in resource_data and "gpu" in resource_data["resource"]:
        for gpu in resource_data["resource"]["gpu"]:
            # GPU 计算使用率：0-60%
            gpu["compute_usage"] = random.randint(0, 60)
            # GPU 内存使用率：20-80%
            mem_usage = round(random.uniform(0.2, 0.8), 2)
            gpu["mem_usage"] = mem_usage
            if "mem_total" in gpu:
                gpu["mem_used"] = int(gpu["mem_total"] * mem_usage)
            # GPU 温度：35-75 度
            gpu["temperature"] = random.randint(35, 75)
            # GPU 功耗：15-50 瓦
            gpu["power"] = random.randint(15, 50)
    
    # 5. 修改网络指标
    if "resource" in resource_data and "network" in resource_data["resource"]:
        for network in resource_data["resource"]["network"]:
            # 网络收发字节数：增加一些随机值
            base_rx = network.get("rx_bytes", 0)
            base_tx = network.get("tx_bytes", 0)
            network["rx_bytes"] = base_rx + random.randint(1000, 100000)
            network["tx_bytes"] = base_tx + random.randint(1000, 100000)
            # 网络速率：1000-50000 KB/s
            network["rx_rate"] = round(random.uniform(1000.0, 50000.0), 2)
            network["tx_rate"] = round(random.uniform(1000.0, 50000.0), 2)
    
    # 6. 修改组件（component）资源指标
    if "component" in resource_data:
        for component in resource_data["component"]:
            if "resource" in component:
                comp_resource = component["resource"]
                
                # 组件 CPU 负载：0-50%
                if "cpu" in comp_resource:
                    comp_resource["cpu"]["load"] = round(random.uniform(0.0, 50.0), 2)
                
                # 组件内存使用率：10-90%
                if "memory" in comp_resource:
                    mem_usage = round(random.uniform(0.1, 0.9), 2)
                    comp_resource["memory"]["mem_usage"] = mem_usage
                    if "mem_limit" in comp_resource["memory"]:
                        mem_limit = comp_resource["memory"]["mem_limit"]
                        comp_resource["memory"]["mem_used"] = int(mem_limit * mem_usage)
                
                # 组件 GPU 内存使用率：0-50%
                if "gpu" in comp_resource:
                    comp_resource["gpu"]["mem_usage"] = round(random.uniform(0.0, 0.5), 2)
                
                # 组件网络指标
                if "network" in comp_resource:
                    comp_resource["network"]["rx"] = random.randint(0, 1000000)
                    comp_resource["network"]["tx"] = random.randint(0, 1000000)
                    comp_resource["network"]["rx_rate"] = round(random.uniform(0.0, 1000.0), 6)
                    comp_resource["network"]["tx_rate"] = round(random.uniform(0.0, 1000.0), 6)
    
    return data


def send_request(url: str, data: Dict[str, Any], endpoint_name: str) -> bool:
    """
    发送 HTTP POST 请求
    """
    try:
        response = requests.post(
            url,
            json=data,
            headers={'Content-Type': 'application/json'},
            timeout=10
        )
        
        if response.status_code == 200:
            print(f"✓ {endpoint_name} 请求成功 (状态码: {response.status_code})")
            return True
        else:
            print(f"✗ {endpoint_name} 请求失败 (状态码: {response.status_code})")
            print(f"  响应: {response.text[:200]}")
            return False
            
    except requests.exceptions.ConnectionError:
        print(f"✗ {endpoint_name} 连接失败：无法连接到服务器 {url}")
        return False
    except requests.exceptions.Timeout:
        print(f"✗ {endpoint_name} 请求超时")
        return False
    except Exception as e:
        print(f"✗ {endpoint_name} 请求异常: {e}")
        return False


def main():
    """主函数"""
    # 解析命令行参数
    host = DEFAULT_HOST
    port = DEFAULT_PORT
    interval = DEFAULT_INTERVAL
    box_start = DEFAULT_BOX_START
    box_end = DEFAULT_BOX_END
    slot_start = DEFAULT_SLOT_START
    slot_end = DEFAULT_SLOT_END
    
    if len(sys.argv) > 1:
        host = sys.argv[1]
    if len(sys.argv) > 2:
        try:
            port = int(sys.argv[2])
        except ValueError:
            print(f"错误：端口号必须是数字，使用默认端口 {DEFAULT_PORT}")
            port = DEFAULT_PORT
    if len(sys.argv) > 3:
        try:
            interval = int(sys.argv[3])
        except ValueError:
            print(f"错误：间隔时间必须是数字，使用默认间隔 {DEFAULT_INTERVAL} 秒")
            interval = DEFAULT_INTERVAL
    if len(sys.argv) > 4:
        try:
            box_start = int(sys.argv[4])
        except ValueError:
            print(f"错误：起始机箱号必须是数字，使用默认值 {DEFAULT_BOX_START}")
            box_start = DEFAULT_BOX_START
    if len(sys.argv) > 5:
        try:
            box_end = int(sys.argv[5])
        except ValueError:
            print(f"错误：结束机箱号必须是数字，使用默认值 {DEFAULT_BOX_END}")
            box_end = DEFAULT_BOX_END
    if len(sys.argv) > 6:
        try:
            slot_start = int(sys.argv[6])
        except ValueError:
            print(f"错误：起始槽位号必须是数字，使用默认值 {DEFAULT_SLOT_START}")
            slot_start = DEFAULT_SLOT_START
    if len(sys.argv) > 7:
        try:
            slot_end = int(sys.argv[7])
        except ValueError:
            print(f"错误：结束槽位号必须是数字，使用默认值 {DEFAULT_SLOT_END}")
            slot_end = DEFAULT_SLOT_END
    
    # 验证参数
    if box_start < 1 or box_end < box_start:
        print(f"错误：机箱号范围无效，使用默认值 {DEFAULT_BOX_START}-{DEFAULT_BOX_END}")
        box_start = DEFAULT_BOX_START
        box_end = DEFAULT_BOX_END
    
    if slot_start < 1 or slot_end < slot_start or slot_end > 14:
        print(f"错误：槽位号范围无效（有效范围1-14），使用默认值 {DEFAULT_SLOT_START}-{DEFAULT_SLOT_END}")
        slot_start = DEFAULT_SLOT_START
        slot_end = DEFAULT_SLOT_END
    
    # 构建 URL
    base_url = f"http://{host}:{port}"
    heartbeat_url = f"{base_url}/heartbeat"
    resource_url = f"{base_url}/resource"
    
    # 计算节点总数
    total_nodes = (box_end - box_start + 1) * (slot_end - slot_start + 1)
    
    print(f"测试脚本启动")
    print(f"服务器地址: {base_url}")
    print(f"发送间隔: {interval} 秒")
    print(f"机箱范围: {box_start}-{box_end}")
    print(f"槽位范围: {slot_start}-{slot_end}")
    print(f"节点总数: {total_nodes}")
    print(f"按 Ctrl+C 停止\n")
    
    # 加载基础测试数据
    print("加载基础测试数据...")
    base_heartbeat_data = extract_json_from_doc(HEARTBEAT_DOC)
    base_resource_data = extract_json_from_doc(RESOURCE_DOC)
    
    if not base_heartbeat_data:
        print("错误：无法加载 heartbeat 测试数据")
        sys.exit(1)
    
    if not base_resource_data:
        print("错误：无法加载 resource 测试数据")
        sys.exit(1)
    
    print("基础测试数据加载成功\n")
    
    # 生成所有节点配置列表
    node_configs: List[Tuple[int, int]] = []
    for box_id in range(box_start, box_end + 1):
        for slot_id in range(slot_start, slot_end + 1):
            node_configs.append((box_id, slot_id))
    
    # 主循环
    request_count = 0
    try:
        while True:
            request_count += 1
            print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] 第 {request_count} 轮请求（共 {total_nodes} 个节点）")
            
            # 遍历所有节点
            success_count = 0
            fail_count = 0
            
            for box_id, slot_id in node_configs:
                # 生成节点数据
                heartbeat_data, resource_data = generate_node_data(
                    base_heartbeat_data,
                    base_resource_data,
                    box_id,
                    slot_id
                )
                
                # 随机化 resource 数据中的指标数值
                randomized_resource_data = randomize_resource_metrics(resource_data)
                
                # 获取正确的 host_ip（用于显示）
                host_ip = calculate_host_ip(box_id, slot_id)
                
                # 发送 heartbeat 请求
                heartbeat_success = send_request(
                    heartbeat_url, 
                    heartbeat_data, 
                    f"/heartbeat (box{box_id}-slot{slot_id}, {host_ip})"
                )
                
                # 发送 resource 请求
                resource_success = send_request(
                    resource_url, 
                    randomized_resource_data, 
                    f"/resource (box{box_id}-slot{slot_id}, {host_ip})"
                )
                
                if heartbeat_success and resource_success:
                    success_count += 1
                else:
                    fail_count += 1
                
                # 短暂延迟，避免请求过快
                time.sleep(0.1)
            
            print(f"本轮完成：成功 {success_count}/{total_nodes}，失败 {fail_count}/{total_nodes}")
            print()  # 空行分隔
            
            # 等待指定时间
            time.sleep(interval)
            
    except KeyboardInterrupt:
        print(f"\n\n脚本已停止，共发送 {request_count} 轮请求（每轮 {total_nodes} 个节点）")


if __name__ == "__main__":
    main()

