{"api_version":1,"data":{"manager_ip":"192.168.10.58","manager_port":18888,"url":"/resource"}}

{
    "api_version": 1,
    "data": {
        "component": [
            {
                "config": {
                    "id": "bc6ebfa09c8d246da4e85b17e79a12fe78a5bf503099d2318b38c62064f12e80",
                    "name": "192.168.10.58:5000/ship:latest"
                },
                "index": 2,
                "instance_id": "d62c38f9-4ba4-45a8-8eb5-1a94d72f1180",
                "resource": {
                    "cpu": {
                        "load": 0.2538899878493317
                    },
                    "gpu": {
                        "mem_usage": 0
                    },
                    "memory": {
                        "mem_limit": 15645540352,
                        "mem_usage": 0.07120955716033041,
                        "mem_used": 11141120
                    },
                    "network": {
                        "rx": 0,
                        "rx_rate": 0,
                        "tx": 0,
                        "tx_rate": 0
                    }
                },
                "state": "RUNNING",
                "type": "docker",
                "uuid": "1935956958418964480"
            },
            {
                "config": {
                    "id": "3383a02f165df570c46f3540424564b19b0f4cb24df797e7d08f2fcd88101828",
                    "name": "192.168.10.58:5000/pts1:latest"
                },
                "index": 1,
                "instance_id": "d62c38f9-4ba4-45a8-8eb5-1a94d72f1180",
                "resource": {
                    "cpu": {
                        "load": 10.847562553191489
                    },
                    "gpu": {
                        "mem_usage": 0
                    },
                    "memory": {
                        "mem_limit": 15645540352,
                        "mem_usage": 0.7238242045473585,
                        "mem_used": 113246208
                    },
                    "network": {
                        "rx": 0,
                        "rx_rate": 0,
                        "tx": 8150,
                        "tx_rate": 4.613493501072183e-06
                    }
                },
                "state": "RUNNING",
                "type": "docker",
                "uuid": "1935950611212275712"
            }
        ],
        "host_ip": "192.168.10.58",
        "resource": {
            "cpu": {
                "core_allocated": 5,
                "core_count": 8,
                "core_unbound": [
                    {
                        "list": [
                            0,
                            1,
                            2,
                            3,
                            4,
                            5,
                            6,
                            7
                        ],
                        "node": 0
                    }
                ],
                "current": 0,
                "load_avg_15m": 0.87,
                "load_avg_1m": 0.65,
                "load_avg_5m": 0.72,
                "power": 0,
                "temperature": 0,
                "usage_percent": 20.973557692307693,
                "voltage": 0
            },
            "disk": [
                {
                    "device": "/dev/sda4",
                    "free": 28978667520,
                    "mount_point": "/",
                    "total": 75125227520,
                    "usage_percent": 61.42618335194361,
                    "used": 46146560000
                },
                {
                    "device": "/dev/sda5",
                    "free": 244199481344,
                    "mount_point": "/data",
                    "total": 426474012672,
                    "usage_percent": 42.73989174299041,
                    "used": 182274531328
                }
            ],
            "gpu": [
                {
                    "compute_usage": 0,
                    "free": 1,
                    "index": 0,
                    "mem_total": 17179869184,
                    "mem_usage": 1,
                    "mem_used": 67108864,
                    "name": "Iluvatar MR-V50A",
                    "power": 19,
                    "temperature": 42
                }
            ],
            "gpu_allocated": 0,
            "gpu_num": 1,
            "memory": {
                "free": 8799125504,
                "total": 15645540352,
                "usage_percent": 43.759529514267044,
                "used": 6846414848
            },
            "network": [
                {
                    "interface": "enaphyt4i0",
                    "rx_bytes": 3034480513,
                    "rx_drop_rate": 5.224804256627129e-06,
                    "rx_errors": 0,
                    "rx_packets": 19139473,
                    "rx_rate": 10888.75,
                    "state": 0,
                    "tx_bytes": 505119675,
                    "tx_drop_rate": 0,
                    "tx_errors": 0,
                    "tx_packets": 2109901,
                    "tx_rate": 9001.5
                },
                {
                    "interface": "docker0",
                    "rx_bytes": 11910301,
                    "rx_drop_rate": 0,
                    "rx_errors": 0,
                    "rx_packets": 29051,
                    "rx_rate": 51831.5,
                    "state": 0,
                    "tx_bytes": 4242345,
                    "tx_drop_rate": 0,
                    "tx_errors": 0,
                    "tx_packets": 38218,
                    "tx_rate": 17285
                }
            ]
        }
    }
}
