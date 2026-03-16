/**
 ****************************************************************************************
 * @file rc_scheduler.h
 * @brief Remote Control Task Scheduler (基于PDF低功耗框架的task_list实现)
 ****************************************************************************************
 */
#ifndef __RC_SCHEDULER_H__
#define __RC_SCHEDULER_H__

#include <stdint.h>
#include "rc_protocol.h"

/**
 * @brief RC调度器状态结构体
 */
typedef struct {
    /* 设备地址 */
    uint8_t esc_addr[5];        /* 电控地址 */

    /* 遥控下行数据 (发送给电控) */
    rc_ctrl_t       ctrl;       /* 当前控制数据: gear/mode/throttle */
    proto_tracker_t tracker;    /* seq跟踪 + 油门确认 */

    /* 电控上行数据 (从ACK payload解析) */
    mc_status_t     mc_status;  /* 电控回传: speed/mileage */
    uint8_t         mc_online;  /* 电控在线标志 (收到有效ACK=1) */

    /* 油门状态 */
    uint8_t  throttle_changed;  /* 油门变化标志 */

    /* 关机标志 */
    uint8_t shutdown_flag;

    /* 运行状态 */
    uint8_t initialized;
} RC_Scheduler_t;

/**
 * @brief 初始化RC调度器
 * @param sched 调度器结构体指针
 */
void RC_Scheduler_Init(RC_Scheduler_t *sched);

/**
 * @brief RC调度器主循环（内部while(1)，不会返回）
 * @param sched 调度器结构体指针
 * @note 任务列表:
 *       - 10ms:  按键扫描
 *       - 20ms:  RF服务 + 解析ACK
 *       - 50ms:  油门更新 + 打包发送控制帧
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
