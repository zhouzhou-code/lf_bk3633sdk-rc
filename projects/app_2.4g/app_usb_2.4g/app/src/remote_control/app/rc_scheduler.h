/**
 ****************************************************************************************
 * @file rc_scheduler.h
 * @brief Remote Control Task Scheduler
 ****************************************************************************************
 */
#ifndef __RC_SCHEDULER_H__
#define __RC_SCHEDULER_H__

#include <stdint.h>

/**
 * @brief RC调度器状态结构体
 */
typedef struct {
    /* 系统状态 */
    uint8_t shutdown_flag;
    uint8_t initialized;
} RC_Scheduler_t;

/**
 * @brief 初始化RC调度器
 */
void RC_Scheduler_Init(RC_Scheduler_t *sched);

/**
 * @brief RC调度器主循环（内部while(1)，不会返回）
 * @note 任务列表:
 *       - 10ms:  按键扫描
 *       - 20ms:  通信层处理RX队列
 *       - 50ms:  控制层更新 + 通信层发送
 *       - 100ms: LCD刷新
 *       - 200ms: 关机检测
 *       - 500ms: 心跳/链路检测
 *       - 700ms: 电池状态查询
 */
void RC_Scheduler_Task(RC_Scheduler_t *sched);

/**
 * @brief 配置电控地址
 */
void RC_Scheduler_SetESCAddr(RC_Scheduler_t *sched, uint8_t *addr);

#endif /* __RC_SCHEDULER_H__ */
