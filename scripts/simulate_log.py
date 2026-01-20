#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
日志模拟脚本
快速生成节点存活状态检查和告警评估的日志输出

python3 simulate_log.py --hours 25 --start-time "2026-01-20 23:50:23" --output logs/yw2.log
"""

import random
from datetime import datetime, timedelta
from typing import List, Tuple


class LogSimulator:
    def __init__(self, start_time: datetime = None, hours: int = 25):
        """
        初始化日志模拟器
        start_time: 起始时间，如果为None则使用当前时间
        hours: 要生成的日志时长（小时）
        """
        self.alive_check_thread_id = 346915929
        self.alert_eval_thread_id = 346915928
        
        # 告警统计数据（模拟数据库中的状态）
        self.firing_count = 369
        self.pending_count = 247
        
        # 时间间隔配置
        self.alive_check_interval = 5.0  # 节点存活检查间隔（秒）
        self.alert_eval_interval = 7.0    # 告警评估间隔（秒）
        
        # 设置起始时间和结束时间
        if start_time is None:
            # 从日志最后的时间开始：2026-01-20 23:50:23
            self.start_time = datetime(2026, 1, 20, 23, 50, 23)
        else:
            self.start_time = start_time
        
        self.end_time = self.start_time + timedelta(hours=hours)
        
    def format_timestamp(self, dt: datetime) -> str:
        """格式化时间戳"""
        return dt.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
    
    def log(self, timestamp: datetime, level: str, thread_id: int, message: str):
        """输出日志"""
        ts_str = self.format_timestamp(timestamp)
        print(f"[{ts_str}] [{level}] [{thread_id}] {message}")
    
    def generate_alive_check_logs(self, start_time: datetime) -> List[Tuple[datetime, str, int, str]]:
        """生成一次节点存活状态检查的日志"""
        logs = []
        # 给开始时间添加随机的毫秒偏移（0-999毫秒），使微秒部分随机
        milliseconds_offset = random.randint(0, 999)  # 毫秒单位
        current_time = start_time + timedelta(milliseconds=milliseconds_offset)
        
        # 开始检查
        logs.append((current_time, "debug", self.alive_check_thread_id, 
                    "=== 开始检查节点存活状态 ==="))
        
        # 模拟检查耗时：1-20毫秒
        check_duration = random.randint(1, 20)
        current_time += timedelta(milliseconds=check_duration)
        
        # 找到的节点数量（当前为72）
        node_count = 72
        logs.append((current_time, "debug", self.alive_check_thread_id, 
                    f"找到 {node_count} 个节点"))
        
        # 检查完成
        alert_count = 0
        logs.append((current_time, "debug", self.alive_check_thread_id, 
                    f"节点存活状态检查完成，耗时: {check_duration} 毫秒，创建/更新了 {alert_count} 个告警"))
        
        return logs
    
    def generate_alert_eval_logs(self, start_time: datetime) -> List[Tuple[datetime, str, int, str]]:
        """生成一次告警评估的日志"""
        logs = []
        current_time = start_time
        
        # 开始告警评估
        logs.append((current_time, "debug", self.alert_eval_thread_id, 
                    "=== 开始告警评估 ==="))
        
        # 生成告警数量（当前为0）
        generated_count = 0
        logs.append((current_time, "debug", self.alert_eval_thread_id, 
                    f"生成了 {generated_count} 个告警"))
        
        # 模拟数据库查询耗时：20-50毫秒
        query_duration = random.randint(20, 50)
        current_time += timedelta(milliseconds=query_duration)
        
        # 数据库中的告警统计（模拟固定值，但可以有小幅波动）
        firing_count = self.firing_count + random.randint(-2, 2)
        pending_count = self.pending_count + random.randint(-2, 2)
        
        # 模拟处理耗时：1500-1900毫秒
        process_duration = random.randint(1500, 1900)
        current_time += timedelta(milliseconds=process_duration)
        
        # 处理状态更新
        processed_count = 0
        logs.append((current_time, "debug", self.alert_eval_thread_id, 
                    f"处理了 {processed_count} 个告警状态更新"))
        
        # 更新到数据库
        updated_count = 0
        logs.append((current_time, "debug", self.alert_eval_thread_id, 
                    f"更新了 {updated_count} 个告警到数据库"))
        
        # 评估完成
        logs.append((current_time, "debug", self.alert_eval_thread_id, 
                    f"告警评估完成，耗时: {process_duration} 毫秒"))
        
        return logs
    
    def generate_all_logs(self):
        """生成所有日志"""
        all_logs = []
        
        # 初始化两个任务的时间
        next_alive_check_time = self.start_time
        next_alert_eval_time = self.start_time + timedelta(seconds=2)  # 告警评估稍晚开始
        
        # 生成日志直到结束时间
        while next_alive_check_time < self.end_time or next_alert_eval_time < self.end_time:
            # 决定下一个要执行的任务
            if next_alive_check_time <= next_alert_eval_time and next_alive_check_time < self.end_time:
                # 执行节点存活检查
                logs = self.generate_alive_check_logs(next_alive_check_time)
                all_logs.extend(logs)
                next_alive_check_time += timedelta(seconds=self.alive_check_interval)
            elif next_alert_eval_time < self.end_time:
                # 执行告警评估
                logs = self.generate_alert_eval_logs(next_alert_eval_time)
                all_logs.extend(logs)
                # 告警评估间隔可能有小幅波动（6-8秒）
                eval_interval = random.uniform(6.0, 8.0)
                next_alert_eval_time += timedelta(seconds=eval_interval)
        
        # 按时间戳排序
        all_logs.sort(key=lambda x: x[0])
        
        return all_logs
    
    def output_logs(self, output_file: str = None):
        """输出日志到文件或标准输出"""
        print(f"开始生成日志，从 {self.format_timestamp(self.start_time)} 到 {self.format_timestamp(self.end_time)}", 
              file=__import__('sys').stderr)
        
        all_logs = self.generate_all_logs()
        
        print(f"共生成 {len(all_logs)} 条日志", file=__import__('sys').stderr)
        
        if output_file:
            with open(output_file, 'a', encoding='utf-8') as f:
                for timestamp, level, thread_id, message in all_logs:
                    ts_str = self.format_timestamp(timestamp)
                    f.write(f"[{ts_str}] [{level}] [{thread_id}] {message}\n")
            print(f"日志已写入文件: {output_file}", file=__import__('sys').stderr)
        else:
            for timestamp, level, thread_id, message in all_logs:
                self.log(timestamp, level, thread_id, message)


def main():
    import sys
    
    # 解析命令行参数
    hours = 25
    output_file = None
    start_time_str = None
    
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '--hours' and i + 1 < len(sys.argv):
            hours = float(sys.argv[i + 1])
            i += 2
        elif sys.argv[i] == '--output' and i + 1 < len(sys.argv):
            output_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--start-time' and i + 1 < len(sys.argv):
            start_time_str = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--help':
            print("用法: simulate_log.py [选项]")
            print("选项:")
            print("  --hours N          生成N小时的日志（默认: 25）")
            print("  --output FILE      输出到文件（默认: 标准输出）")
            print("  --start-time TIME  起始时间，格式: YYYY-MM-DD HH:MM:SS（默认: 从日志最后时间开始）")
            print("  --help             显示帮助信息")
            sys.exit(0)
        else:
            i += 1
    
    # 解析起始时间
    start_time = None
    if start_time_str:
        try:
            start_time = datetime.strptime(start_time_str, "%Y-%m-%d %H:%M:%S")
        except ValueError:
            print(f"错误: 无效的时间格式: {start_time_str}", file=sys.stderr)
            print("正确格式: YYYY-MM-DD HH:MM:SS", file=sys.stderr)
            sys.exit(1)
    
    # 创建模拟器并生成日志
    simulator = LogSimulator(start_time=start_time, hours=hours)
    simulator.output_logs(output_file)


if __name__ == "__main__":
    main()
