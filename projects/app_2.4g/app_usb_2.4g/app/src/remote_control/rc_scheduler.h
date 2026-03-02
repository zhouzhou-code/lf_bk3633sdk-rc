#ifndef __RC_SCHEDULER_H__
#define __RC_SCHEDULER_H__

#include <stdint.h>

// RC调度器状态结构体
typedef struct {
    // 超时管理
    uint32_t last_send_time;    // 上次发送时间
    uint32_t rx_timeout_ms;     // RX超时时间（0=不等待）
    uint8_t  tx_seq;            // 发送序列号

    // 设备地址
    uint8_t esc_addr[5];        // 电控地址
    uint8_t bat_addr[5];        // 电池地址

    // 任务时间戳
    uint32_t last_esc_time;     // 上次电控通信时间
    uint32_t last_bat_time;     // 上次电池通信时间

    // 任务周期配置
    uint16_t esc_period_ms;     // 电控通信周期
    uint16_t bat_period_ms;     // 电池通信周期
} RC_Scheduler_t;

// 初始化RC调度器
void RC_Scheduler_Init(RC_Scheduler_t *sched);

// 配置设备地址
void RC_Scheduler_SetESCAddr(RC_Scheduler_t *sched, uint8_t *addr);
void RC_Scheduler_SetBATAddr(RC_Scheduler_t *sched, uint8_t *addr);

// 配置任务周期
void RC_Scheduler_SetESCPeriod(RC_Scheduler_t *sched, uint16_t period_ms);
void RC_Scheduler_SetBATPeriod(RC_Scheduler_t *sched, uint16_t period_ms);

// 发送命令
void RC_Scheduler_SendESCCmd(RC_Scheduler_t *sched, uint16_t throttle);
void RC_Scheduler_SendBATQuery(RC_Scheduler_t *sched);

// 处理接收数据
void RC_Scheduler_ProcessRxData(RC_Scheduler_t *sched);

// RC调度器主任务
void RC_Scheduler_Task(RC_Scheduler_t *sched);

// 检查是否可以进入sleep
uint8_t RC_Scheduler_CanSleep(RC_Scheduler_t *sched);

#endif // __RC_SCHEDULER_H__
