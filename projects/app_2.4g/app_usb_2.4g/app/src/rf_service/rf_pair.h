#ifndef __RF_PAIR_H__
#define __RF_PAIR_H__
#include <stdint.h>


#define PAIR_CH_DEFAULT           78
#define slave_magic_number        0x5A
#define host_magic_number         0xA5

//用户flash：0x7E000~0x7F000
//Flash存储配置配对信息的位置
// 已迁移至 app_addr_manage 模块统一管理

typedef enum {
    SLAVE_PAIR_IDLE = 0,
    SLAVE_PAIR_SEND_REQ,      // 发送配对请求
    SLAVE_PAIR_WAIT_RESP,     // 等待配对响应 (超时即重发)
    SLAVE_PAIR_VERIFY_PHASE,  // 新地址验证 (被动回复 PONG)
    SLAVE_PAIR_DONE
} slave_pair_state_t;


typedef enum {
    HOST_PAIR_IDLE = 0,
    HOST_PAIR_WAIT_REQ,       // 等待配对请求
    HOST_PAIR_SEND_RESP,      // 突发发送响应 (Burst)
    HOST_PAIR_WAIT_TX_DONE,   // 等待发送完成
    HOST_PAIR_VERIFY_PHASE,   // 新地址验证 (主动 PING，超时则回退)
    HOST_PAIR_DONE
} host_pair_state_t;



// 命令码
typedef enum {
    CMD_PAIR_REQ = 0x10,  // Slave -> Host
    CMD_PAIR_RESP = 0x20, // Host -> Slave

    // 握手验证
    CMD_PAIR_VERIFY_PING = 0x40,  // Host -> Slave
    CMD_PAIR_VERIFY_PONG = 0x50   // Slave -> Host
} rf_pair_cmd_t;


/* 地址分配策略 */
#define ADDR_MODE_HOST_ASSIGN   0   /* Host生成随机地址(默认) */
#define ADDR_MODE_SLAVE_PROVIDE 1   /* Slave自带地址 */

#pragma pack(1)
typedef struct {
    uint8_t cmd;            /* CMD_PAIR_REQ (0x10) */
    uint8_t dev_type;       /* 设备类型枚举 (device_type_t: 0-3) */
    uint8_t addr_mode;      /* 地址分配策略 (0=HOST_ASSIGN, 1=SLAVE_PROVIDE) */
    uint8_t addr[5];        /* 从机提供的地址 (addr_mode=1时有效, =0时填0) */
} pair_req_pkt;             /* 总长: 9B */

typedef struct {
    uint8_t cmd;            /* CMD_PAIR_RESP (0x20) */
    uint8_t new_addr[5];    /* 通信地址 */
    uint8_t new_chn;        /* 通信信道 (预留, 当前为0) */
} pair_resp_pkt;            /* 总长: 7B */

typedef struct {
    uint8_t cmd;
    uint8_t magic_number;
} pair_verify_pkt;          /* 总长: 2B */
#pragma pack()


typedef struct {
    slave_pair_state_t state;
    uint32_t start_wait;
    uint32_t global_start_tick; // 总超时计时
    uint8_t is_running;
    uint8_t dev_type;           // 本机设备类型 (device_type_t)
    uint8_t addr_mode;          // 地址分配策略
    uint8_t slave_addr[5];      // SLAVE_PROVIDE模式下的自带地址

    pair_resp_pkt resp_pkt; // 缓存接收到的RESP包
    uint32_t verify_cnt;               // ping-pong成功计数
    uint32_t verify_target_count;      // 目标ping-pong次数
    uint8_t reach_goal_flag;           // 达到目标计数标志
    uint32_t last_ping_recv_timestamp; // 上次收到ping的时间戳
} SlavePairCtrl_t;




typedef struct {
    host_pair_state_t state;
    uint8_t is_running;

    uint32_t start_wait;        // 阶段超时
    uint32_t global_start_tick; // 总超时
    uint32_t last_ping_tick;    // Ping 频率控制

    pair_resp_pkt resp_pkt;     // 缓存发送RESP包
    uint32_t txds_snapshot;     // 发送计数快照

    uint32_t verify_cnt;               // ping-pong成功计数
    uint32_t verify_target_count;      // 目标ping-pong次数，小于slave的目标ping-pong次数
    uint8_t  paired_dev_type;          // 配对的设备类型
} HostPairCtrl_t;

extern uint8_t slave_pair_success_flag;
extern uint8_t host_pair_success_flag;

// 非阻塞配对接口
void Slave_Pairing_Task(uint8_t *flag);
void Host_Pairing_Task(uint8_t *flag);

/**
 * @brief 设置Slave配对参数（在启动配对前调用）
 * @param dev_type  本机设备类型 (DEV_TYPE_ESC / DEV_TYPE_BATTERY / DEV_TYPE_CADENCE)
 * @param addr_mode 地址分配策略 (ADDR_MODE_HOST_ASSIGN / ADDR_MODE_SLAVE_PROVIDE)
 * @param addr      addr_mode=SLAVE_PROVIDE时提供5B地址，HOST_ASSIGN时传NULL
 */
void Slave_Pairing_SetConfig(uint8_t dev_type, uint8_t addr_mode, const uint8_t *addr);

//配对信息存取接口
//已废弃，使用 app_addr_manage 模块接口
//void Load_Pair_Info(void);
//void Save_Pair_Info(uint8_t *addr);
//void Clear_Pair_Info(void);



#endif // __RF_PAIR_H__
