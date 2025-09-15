typedef enum

{

    ERRORTASK=-1,

    RESET,        //复位             负载为ResetTaskT，返回格式为ResetResultInfoT

    SWITCHMODE,   //切换工作模式      负载为SwitchModeTaskT，返回格式为RCommonResultInfoT

    STOPMODE,     //停止工作模式      负载长度为0，返回格式为CommonResultInfoT

    REQNODESTAT,  //请求节点状态      负载长度为0，返回格式为SystemInfoT

    REQCOMPONENT, //请求组件状态      负载为ReqComponentInfoT，返回格式为ComponentDetailsT

    REQBMC,       //BMC信息请求      负载长度为0，返回格式为BMCInfoT

    SELFTEST,     //自检模式，按机箱进行自检，支持多个异步请求         负载为ReqSelfTestInfo，返回格式为SelfTestInfo

    RESERV        //保留位,此保留不可顺移，对于DSP来说是BMC的完整的信息结构体

}TASKID;





//任务单元帧头，共12字节

typedef struct

{

    TASKID m_taskID;    //4字节,任务号

    uint   m_flgVal;    //4字节，＝0

    int    m_buflen;    //4字节，负载长度

}TaskUnit;





typedef struct

{

    char reset_node[9][10]; /*9个机箱1-5,8-12共10个槽位的状态;0:不复位;1:复位;-*/

    int reset_mode;        /*复位模式;1:业务管理子系统接口复位;2:交换板BMC方式复位;*/

}ResetTaskT;


typedef struct

{

    char result[9][12];        /*操作结果*/

    char message[64];  /*操作错误消息描述*/

}ResetResultInfoT;

/* 复位模式 */
typedef enum : uint8_t {
  RESET_MODE_API = 1,   /* 业务管理子系统接口复位 */
  RESET_MODE_BMC = 2    /* 交换板BMC方式复位 */
} ResetModeT;

/* ResetTaskT：按箱位图选择要复位的槽位；每箱12位，1=复位 */
typedef struct
{
  uint32_t  request_id;              /* 回显追踪 */
  uint8_t   reset_mode;              /* ResetModeT */
  uint16_t  slot_bitmap[9];          /* 每箱 12 位：bit0->槽1, bit11->槽12 */
} ResetTaskT;

/* ResetResultInfoT：按箱位图展示复位槽位结果；每箱12位，0=复位成功 */
typedef struct
{
  uint32_t  request_id;              /* 与请求对应 */
  uint16_t  slot_result_bitmap[9];         /* 每箱 12 位：bit0->槽1, bit11->槽12 */
  uint16_t  message_len;             /* 实际消息长度，<= sizeof(message) */
  char      message[64];             /* UTF-8/NUL 终止，长度受 message_len 约束 */
} ResetResultInfoT;




typedef struct

{

    int target_mode;        /*选择工作模式:1;2;3*/

    int switch_mode;        /*0:停止后再启动;1:复位后再启动*/

}SwitchModeTaskT;


/* 切换工作模式：枚举与优化后的请求结构 */
typedef enum : uint8_t {
  WORK_MODE_1 = 1,
  WORK_MODE_2 = 2,
  WORK_MODE_3 = 3
} WorkModeT;

/* 切换策略：0=先停后启；1=复位后启 */
typedef enum : uint8_t {
  SWITCH_POLICY_STOP_START  = 0,
  SWITCH_POLICY_RESET_START = 1
} SwitchPolicyT;

/* 优化后的 SwitchModeTaskT：增加请求跟踪与超时、并使用枚举 */
typedef struct
{
  uint32_t  request_id;              /* 回显追踪，没有可填0 */
  uint8_t   target_mode;             /* WorkModeT：目标工作模式 */
  uint8_t   switch_policy;           /* SwitchPolicyT：切换策略 */
} SwitchModeTaskT;

typedef struct

{

    char result;        /*操作结果*/

    char message[64];  /*操作错误消息描述*/

}CommonResultInfoT;

/* 通用结果码 */
typedef enum : uint8_t {
  RESULT_OK           = 0,
  RESULT_PARTIAL      = 1,  /* 部分成功 */
  RESULT_FAILED       = 2
} ResultCodeT;

/* 优化后的 CommonResultInfoT：带请求ID与消息长度 */
typedef struct
{
  uint32_t  request_id;              /* 与请求对应，无则为0 */
  uint8_t   code;                    /* ResultCodeT */
  uint16_t  message_len;             /* 实际消息长度，<= sizeof(message) */
  char      message[64];             /* UTF-8/NUL 终止 */
} CommonResultInfoT;



//节点状态返回结构体

typedef struct

{

    char nodeStatus[9][10]; /*9个机箱1-5,8-12共10个槽位的状态*/

    char swSatus[9][2];         /*交换板状态，可以考虑去掉*/

    ComponentInfoT componentStatus[9][10][8];/*1-5,8-12共10个槽位的组件状态，组件所在结构体的index就是他的id（0-8）*/

}SystemInfoT;



/* SystemInfo 偏移表（定长组件，offset-only）方案 */

/* 定长组件条目（对齐到4字节） */
typedef struct
{
  uint32_t component_id;   /* 槽位内或全局唯一 */
  uint8_t  status;         /* ComponentStatusT */
  uint8_t  reserved;       /* 对齐/扩展 */
} ComponentBriefV1;

/* 头部 + 偏移表；负载区为各槽组件条目顺序拼接
 * 组件数 = (next_nonzero_offset_or_payload_len - curr_offset) / sizeof(ComponentBriefV1)
 */
typedef struct
{
  uint16_t  valid_slot_bitmap[9];                  /* 每箱 12 位：1=该槽位有效 */
  uint32_t  payload_len;                           /* 负载总长度（字节） */
  uint32_t  slot_payload_offset[9][12];            /* 0=无数据；否则为负载区相对偏移 */
} SystemInfoHeader_OffsetOnlyV1;

typedef struct

{

    char m_status;        /*任务状态 0:正常 -1:异常*/

}ComponentInfoT;





//组件状态请求结构体

typedef struct

{

    int boxId;         /*机箱号*/

    int slotId;        /*槽位号*/

    int index;        /*组件索引号，根据SystemInfoT组件所在的位置*/

}ReqComponentInfoT;





//组件状态返回结构体

typedef struct

{

    uint nodeIp;   /*组件运行节点的IP*/

    int m_status;        /*任务状态 0:正常 -1:异常*/

    uint m_uid;           /*任务uid*/

    uint m_workMode;       /*所属工作模式*/

    uint txSpeed;          /*  B/s */

    uint rxSpeed;          /*  B/s */

    uint cpuUsage;         /*  %   */

    uint mememoryUsage;    /*  MB  */

    uint gpuMememoryUsage; /*  MB  */

}ComponentDetailsT;


/* 组件状态请求（优化版）：按 box/slot + component_id 定位（或索引） */
typedef struct
{
  uint32_t  request_id;        /* 回显追踪，无则为0 */
  uint8_t   box_id;            /* [0..8] */
  uint8_t   slot_id;           /* [0..11] */
  uint32_t  component_id;      /* 槽位内组件ID；当 flags.bit0=1 时表示索引 */
} ReqComponentInfoV2;

/* 组件状态返回（优化版）：固定宽度类型与单位明确 */
typedef struct
{
  uint32_t  request_id;            /* 与请求对应，无则为0 */
  uint32_t  component_id;          /* 回显组件ID（或由索引映射得到） */
  uint8_t   status;                /* 组件状态码（0=OK，其他自定义） */
  uint8_t   work_mode;             /* WorkModeT，如无可为0 */
  uint32_t  node_ip;               /* IPv4，网络字节序 */
  uint32_t  tx_speed_Bps;          /* 发送速率 B/s */
  uint32_t  rx_speed_Bps;          /* 接收速率 B/s */
  uint16_t  cpu_usage_permil;      /* CPU 利用率，千分比 0..1000 */
  uint32_t  memory_usage_MB;       /* 内存占用 MB */
  uint32_t  gpu_memory_usage_MB;   /* GPU 内存占用 MB */
} ComponentDetailsV2;



//BMC信息返回结构体

typedef struct

{

    float temp1[9][12];

    float temp2[9][12];

    float vol1[9][12];

    float vol2[9][12];

    float cur2[9][12];

}BMCInfoT;



//自检请求结构体

typedef struct

{

    int m_box;

    char m_slot[16]; //m_slot[x] x=槽位号，0对应1槽，11对应第12槽;0:不执行自检;1:执行自检

    int m_reqId;     //每次请求给不同的序号，要求和请求一致

}ReqSelfTestInfo;



//自检请求结构体

typedef struct

{

    int m_box;

    char m_slot[16]; //m_slot[x] x=槽位号，0对应1槽，11对应第12槽

    char m_type[16]; //不同板卡类型,参考本文件宏定义

    int m_reqId;     //每次请求给不同的序号，要求和请求一致

}SelfTestInfo;


/* BMC 信息（优化版）：固定宽度 + 明确单位 + 槽位位图 */
typedef struct
{
  uint16_t  valid_slot_bitmap[9];         /* 每箱 12 位：1=该槽位有效 */
  int16_t   temp1_tenth_c[9][12];         /* 温度 x0.1 °C，可表示负温 */
  int16_t   temp2_tenth_c[9][12];
  uint16_t  vol1_mV[9][12];               /* 毫伏 */
  uint16_t  vol2_mV[9][12];
  uint16_t  cur2_mA[9][12];               /* 毫安 */
} BMCInfoV2;

/* 自检请求（优化版）：位图选择槽位，支持按板卡类型 */
typedef struct
{
  uint32_t  request_id;                   /* 回显追踪，无则为0 */
  uint8_t   box_id;                       /* [0..8] */
  uint16_t  slot_bitmap;                  /* 12 位：bit0->槽1, bit11->槽12 */
} ReqSelfTestInfoV2;

/* 自检结果（优化版）：逐槽位结果 + 文本消息 */
typedef struct
{
  uint32_t  request_id;                   /* 与请求对应，无则为0 */
  uint8_t   box_id;                       /* [0..8] */
  uint16_t  slot_result_bitmap;              /* SelfTestSlotResultT */
  uint8_t   m_type[16]; //不同板卡类型,参考本文件宏定义
} SelfTestInfoV2;
