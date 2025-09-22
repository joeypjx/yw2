#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// 核心 OpenIPMI 头文件 [cite: 3303]
#include <OpenIPMI/ipmiif.h>
#include <OpenIPMI/ipmi_lan.h>
#include <OpenIPMI/ipmi_posix.h>
#include <OpenIPMI/ipmi_err.h>
#include <OpenIPMI/ipmi_auth.h> // 用于认证类型宏 [cite: 3303]

// 用于保存需要跨回调函数传递的数据
typedef struct {
    ipmi_domain_t *domain;
    os_handler_t *os_hnd;
} app_data_t;

// 响应处理器，当收到IPMI响应时被调用 [cite: 2334]
static void response_handler(ipmi_domain_t *domain,
                             ipmi_addr_t *addr,
                             unsigned int addr_len,
                             ipmi_msg_t *msg,
                             void *rsp_data1,
                             void *rsp_data2)
{
    printf("Response received.\n");
    // 每个响应的第一个字节是完成码 [cite: 2289, 2302]
    if (msg->data_len > 0) {
        printf("  Completion Code: 0x%02x\n", msg->data[0]);
    } else {
        printf("  Response has no data (zero length).\n");
        return;
    }

    if (msg->data_len > 1) {
        printf("  Response Data (%d bytes):", msg->data_len - 1);
        for (int i = 1; i < msg->data_len; i++) {
            printf(" 0x%02x", msg->data[i]);
        }
        printf("\n");
    }

    // 收到响应后，可以停止事件循环
    // 在实际应用中，您可能有更复杂的逻辑来决定何时退出
    // 为了简单起见，这里我们直接退出。
    exit(0);
}

// 当域(domain)完全建立连接后，此回调函数被调用 [cite: 3342]
static void setup_done(ipmi_domain_t *domain,
                       int err,
                       unsigned int conn_num,
                       unsigned int port_num,
                       int still_connected,
                       void *user_data)
{
    app_data_t *app = (app_data_t *)user_data;
    int rv;

    if (err || !still_connected) {
        fprintf(stderr, "Connection failed: 0x%x\n", err);
        exit(1);
    }
    printf("Connection to domain established.\n");

    // 1. 构造桥接地址 (IPMB 地址) [cite: 2148]
    struct ipmi_ipmb_addr target_addr;
    target_addr.addr_type = IPMI_IPMB_ADDR_TYPE; // 设置地址类型为 IPMB [cite: 2149]
    target_addr.channel = 1;      // 对应 -b 1 参数 (桥接通道) 
    target_addr.slave_addr = 0x06;  // 对应 -t 0x06 参数 (目标地址) 
    target_addr.lun = 0;          // 通常为0

    // 2. 构造 Raw IPMI 消息
    struct ipmi_msg msg;
    unsigned char msg_data[] = {0x01, 0x68, 0x68, 0x03, 0x00, 0x02, 0xD0};
    msg.netfn = 0x2e;              // Raw 命令的网络功能码 [cite: 885, 890]
    msg.cmd = 0x11;                // Raw 命令的命令码 [cite: 886, 891]
    msg.data = msg_data;           // 指向数据负载的指针 [cite: 888]
    msg.data_len = sizeof(msg_data); // 数据负载的长度 [cite: 887]

    printf("Sending RAW command to Channel %d, Address 0x%02x...\n",
           target_addr.channel, target_addr.slave_addr);
    
    // 3. 发送命令 
    rv = ipmi_send_command_addr(domain,
                                (ipmi_addr_t *)&target_addr,
                                sizeof(target_addr),
                                &msg,
                                response_handler, // 指定响应回调函数 [cite: 2334]
                                NULL,
                                NULL);
    if (rv) {
        fprintf(stderr, "Error sending command: %s (0x%x)\n", strerror(rv), rv);
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    os_handler_t *os_hnd;
    ipmi_con_t *con;
    int rv;
    app_data_t app;

    // 连接参数
    char *ip_addrs[] = {"192.180.0.213"};
    char *ports[] = {"623"}; // IPMI LAN 默认端口是 623 [cite: 1488]
    char *username = "root";
    char *password = "0penBmc";
    // lanplus 接口通常使用 MD5 认证 
    unsigned int authtype = IPMI_AUTHTYPE_MD5; 
    // root 用户通常需要 admin 权限 [cite: 3305, 1492]
    unsigned int privilege = IPMI_PRIVILEGE_ADMIN; 

    // 1. 分配并设置一个 OS handler [cite: 3351]
    os_hnd = ipmi_posix_setup_os_handler();
    if (!os_hnd) {
        fprintf(stderr, "Unable to allocate os handler\n");
        exit(1);
    }
    app.os_hnd = os_hnd;

    // 2. 初始化 OpenIPMI 库 [cite: 3353]
    ipmi_init(os_hnd);

    // 3. 设置一个 LAN 连接 [cite: 1466]
    rv = ipmi_ip_setup_con(ip_addrs,
                           ports,
                           1, // IP 地址数量 [cite: 1490]
                           authtype, // 认证类型 [cite: 1491]
                           privilege, // 权限级别 [cite: 1492]
                           username, // 用户名 [cite: 1493]
                           strlen(username), // 用户名长度 [cite: 1494]
                           password, // 密码 [cite: 1495]
                           strlen(password), // 密码长度 [cite: 1496]
                           os_hnd, // OS handler [cite: 1497]
                           NULL,
                           &con); // 返回的新连接 [cite: 1500]
    if (rv) {
        fprintf(stderr, "ipmi_ip_setup_con failed: %s (0x%x)\n", strerror(rv), rv);
        exit(1);
    }

    // 4. 使用创建的连接打开一个域 [cite: 741]
    rv = ipmi_open_domain("", &con, 1, setup_done, &app, NULL, NULL,
                          NULL, 0, NULL);
    if (rv) {
        fprintf(stderr, "ipmi_open_domain failed: %s (0x%x)\n", strerror(rv), rv);
        exit(1);
    }
    
    // 5. 运行事件循环来处理 I/O 操作 [cite: 3363]
    printf("Waiting for connection and response...\n");
    while (1) {
        // 处理一个事件 (文件操作或定时器超时) 然后返回 [cite: 390]
        os_hnd->perform_one_op(os_hnd, NULL);
    }

    // 清理资源 (虽然在上面的无限循环中不会到达这里，但这是良好实践)
    os_hnd->free_os_handler(os_hnd); [cite: 3705]
    return 0;
}