GET /node/export

{
    "start_time":"秒级时间戳",
    "end_time":"秒级时间戳",
    "ip":["192.168.10.58"],
    "type":["system"],
}

{
code: "200/400/"
data: [
    start_time:"xx-xx-xx xx:xx:xx",
    end_time:"xx-xx-xx xx:xx:xx",
    ip:"192.168.10.58",
    type:["cpu_usage_percent",“memory_usage_percent”,...],
    data:[
         {
            timestamp:"xx-xx-xx xx:xx:xx"
            cpu_usage_percent:20,
            memory_usage_percent: 11,
            disk_/_usage_percent: 20,
            disk_/data_usage_percent: 20,
            network_eth0_rx_rate: 2222,
            network_eth0_tx_rate: 2222,
            gpu_0_compute_usage: 23,
            gpu_0_mem_usage: 23
         }
        ]
    ]
}
