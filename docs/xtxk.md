#pragma pack(push, 1)

#include <stdint.h> // 改进：引入固定宽度整数类型
#include <stdbool.h> // 改进：引入布尔类型

#define JXC_MAX_CHASSIS                9   // 最大机箱数量
#define JXC_MAX_SLOTS_PER_CHASSIS      16  // 最大槽位数量 (统一为16以支持位掩码)
#define JXC_MAX_COMPONENTS_PER_SLOT    8   // 每个槽位的最大组件数
#define JXC_MAX_MESSAGE_LEN            64  // 消息字符串最大长度

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
  uint32_t  slot_payload_offset[9][12];            /* next_nonzero_offset_or_payload_len - curr_offset=无数据；否则为负载区相对偏移 */
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

#pragma pack(pop)

### 可以优化的要点（针对你给出的这些 C 风格数据结构）

- **使用确定宽度的整数类型**
  - 用 `uint8_t/uint16_t/uint32_t/int32_t` 替代 `char/int/uint`，避免平台差异与符号不确定性。
  - `IP` 用 `uint32_t`（网络字节序）或单独的 `struct { uint32_t v4; }` 表示；不要用模糊的 `uint`。

- **枚举更规范与可扩展**
  - 给 `TASKID` 指定底层类型（如 `enum TASKID : uint16_t`），显式赋值并预留区间；避免把错误态设为负数且与无符号混用。
  - `RESET/SWITCHMODE/...` 对应的“模式值”用独立 `enum class` 表达，别用裸 `int`。

- **尺寸常量与索引一致**
  - 统一“机箱数/槽位数/组件数”的常量定义（如 `kNumBoxes, kNumSlots, kNumComponents`），避免 `[9][10]` 和 `[9][12]`、`[16]` 的混乱。
  - 清晰定义槽位是 0 基还是 1 基，并在注释和结构中保持一致。若逻辑上只用 1-5、8-12，明确映射策略：要么存 `[12]` 并标记无效位，要么压缩为 `[10]` 并提供映射表。

- **布尔/状态表达更明确**
  - 不用 `char` 表达布尔或状态，改为 `uint8_t`/`bool` 或小枚举（如 `enum class Status : uint8_t { Ok=0, Error=1 }`）。
  - 多槽位选择（如 `m_slot[16]`）改为位图或 `std::bitset`/`uint16_t` 掩码，节省空间且表达清晰。

- **结构体布局与对齐**
  - 字段顺序按“从小到大”或“同类型聚合”减少填充；对跨平台二进制协议不要依赖编译器布局，一律走显式序列化。
  - 如需强制布局，使用单独的“线协议结构体”并显式 pack，同时保留“内存域模型结构体”用于业务逻辑。

- **通用结果模型**
  - `ResetResultInfoT/CommonResultInfoT` 统一为通用结果头（如 `code`、`message_len`、`message`），细化内容通过可选字段或后续负载承载，避免为每类动作复制粘贴一套。

- **任务头（TaskUnit）可扩展**
  - 为头部增加：`version`、`seq/request_id`、`flags`（定义位意义）、`payload_len`、可选 `checksum/CRC`。
  - `m_flgVal` 不要固定“=0”；定义明确的比特位语义（是否异步、是否压缩、是否分片等）。

- **自检结构去重与命名**
  - 两个“自检请求结构体”应语义区分并改名（如 `SelfTestRequest` 与 `SelfTestReport`），字段一致的放一起，差异的拆开；`m_type` 用枚举。

- **浮点与单位**
  - `BMCInfoT` 的温度/电压/电流建议定义单位与精度；若走线协议，优先采用定点整数（如 x0.1 摄氏度）以消除平台浮点差异。

- **字符串与编码**
  - `message[64]` 明确 UTF-8 与 NUL 终止策略；最好携带长度字段避免溢出。若未来需要本地化，建议独立错误码表，消息作为可选调试字段。

- **组件/资源标识**
  - 用强类型（如 `BoxId/SlotId/ComponentId` 的 `typedef` 或 `using`）替代裸 `int`；`index` 的语义和来源在注释与校验中固定下来。

- **序列化与端序**
  - 明确“网络字节序”并在编解码时统一转换；禁止直接 `memcpy` 结构体。为每个结构提供成对 `encode/decode` 函数与 `static_assert(sizeof(...))` 校验。

- **容量与传输优化**
  - 超大矩阵（如 `[9][12]`×多类传感器）考虑：只传变化值、分页/分片、或按需查询；支持压缩（如 zstd）可用头部 `flags` 标记。

- **命名与风格一致**
  - 统一命名风格（全用 `snake_case` 或 `lowerCamelCase`）；前缀 `m_` 的用法保持一致；英文拼写修正（如 `swSatus`→`swStatus`、`mememory`→`memory`）。

- **错误与版本策略**
  - 建立稳定的错误码枚举和协议版本迁移策略；头部包含 `version`，字段新增用“TLV/可选段”或“尾部扩展长度”避免破坏兼容。

- **考虑 Schema/IDL**
  - 若协议复杂且跨语言，考虑使用 Protobuf/FlatBuffers/Cap’n Proto 这类 IDL，自动生成编码、版本与兼容处理，减少手工对齐与端序风险。

### 需要特别留意的不一致点
- 槽位维度在不同结构中有 `[10]`、`[12]`、`[16]` 三种表达，且注释含“1-5, 8-12 共10个槽位”，应统一常量与映射。
- `SelfTest` 结构体重复且注释相同，语义混淆。
- 多处使用 `char` 表达状态/布尔与数组，建议改为枚举/位图。
- 字段拼写与大小写风格不一，影响长期可维护性。

以上优化能提升跨平台稳定性、内存与带宽效率、以及协议的前向/后向兼容能力。