/**
 ****************************************************************************************
 * @file rc_scheduler.c
 * @brief Remote Control Task Scheduler
 *
 * 调度流程:
 *   50ms油门更新 → 检测变化 → 打包控制帧 → 入RF发送队列
 *   20ms RF服务  → 发送队列中的帧 → 收到ACK → 解析电控状态
 *   收到ACK后更新tracker，确认油门送达则停止重发
 *
 ****************************************************************************************
 */
#include "rc_scheduler.h"
#include "rc_protocol.h"
#include "app_shutdown.h"
#include "app_sleep.h"
#include "app_key.h"
#include "app_throttle.h"
#include "rf_handler.h"
#include "timer_handler.h"
#include "user_config.h"
#include <string.h>

/* RF活动后的保护窗口(ms)，等待发送完成+接收ACK */
#define RF_GUARD_WINDOW_MS  8

/* ======================== 内部函数 ======================== */

/*
 * 打包控制帧并入RF发送队列
 * 每次调用seq自增，油门变化时标记pending
 */
static void send_ctrl_frame(RC_Scheduler_t *sched)
{
    uint8_t tx_buf[32];
    uint8_t seq = tracker_next_seq(&sched->tracker);
    uint8_t len = proto_pack_ctrl(tx_buf, seq, &sched->ctrl);

    RF_txQueue_Send(sched->esc_addr, tx_buf, len);
}

/*
 * 处理RF接收队列: 解析电控ACK payload
 * 成功解析后更新mc_status和tracker确认状态
 */
static void process_rx_queue(RC_Scheduler_t *sched)
{
    const uint8_t *rx_data;
    uint8_t rx_len, pipes;

    while (RF_rxQueue_Recv(&rx_data, &rx_len, &pipes)) {
        uint8_t ack_seq;
        mc_status_t status;

        if (proto_parse_status(rx_data, rx_len, &ack_seq, &status) == 0) {
            /* 解析成功: 更新电控状态 */
            sched->mc_status = status;
            sched->mc_online = 1;

            /* 更新确认跟踪 */
            tracker_on_ack(&sched->tracker, ack_seq);
        }
    }
}

/* ======================== 公开接口 ======================== */

void RC_Scheduler_Init(RC_Scheduler_t *sched)
{
    memset(sched, 0, sizeof(RC_Scheduler_t));

    /* 锁定电源 */
    app_board_power_on();

    /* 初始化各模块 */
    app_throttle_init();
    app_key_init();

    /* 初始化协议跟踪器 */
    tracker_init(&sched->tracker);

    /* 默认控制数据 */
    sched->ctrl.gear = 1;
    sched->ctrl.mode = MODE_ASSIST;
    sched->ctrl.throttle = 0;

    sched->initialized = 1;
    uart_printf("RC_Scheduler_Init done\r\n");
}

void RC_Scheduler_Task(RC_Scheduler_t *sched)
{
    if (!sched->initialized) return;

    uart_printf("enter RC_Scheduler_Task\r\n");

    uint32_t ts[8] = {0};              /* 各任务时间戳 */
    uint32_t rf_guard_deadline = 0;    /* RF保护窗口截止时间 */

    while (1) {
        uint32_t now = Get_SysTick_ms();

        /* ========== 10ms: 按键扫描 ========== */
        if (now - ts[0] >= 10) {
            ts[0] = now;
            app_key_scan();
        }

        /* ========== 20ms: RF服务 + 解析ACK ========== */
        if (now - ts[1] >= 20) {
            ts[1] = now;

            /* 先处理接收队列(上一轮的ACK) */
            process_rx_queue(sched);

            /* RF收发服务 */
            RF_Service_Handler(&hrf);
        }

        /* ========== 50ms: 油门更新 + 发送控制帧 ========== */
        if (now - ts[2] >= 50) {
            ts[2] = now;

            uint16_t throttle_val;
            uint8_t  changed;
            app_throttle_update(&throttle_val, &changed);

            sched->ctrl.throttle = (uint8_t)throttle_val;

            if (changed) {
                sched->throttle_changed = 1;
                tracker_mark_pending(&sched->tracker);
            }

            /*
             * 发送条件:
             *   1. 油门待确认(pending) → 必须持续发送直到ACK确认
             *   2. 正常周期发送 → 即使油门没变也定期发(保持链路)
             */
            send_ctrl_frame(sched);
        }

        /* ========== 100ms: LCD刷新 ========== */
        if (now - ts[3] >= 100) {
            ts[3] = now;
            /* TODO: app_lcd_refresh(&sched->mc_status); */
        }

        /* ========== 200ms: 关机检测 ========== */
        if (now - ts[4] >= 200) {
            ts[4] = now;
            app_board_shutdown(sched->shutdown_flag);
        }

        /* ========== 500ms: 心跳/链路检测 ========== */
        if (now - ts[5] >= 500) {
            ts[5] = now;
            /* TODO: app_heartbeat_send(); */
            /* TODO: app_check_link(); */
        }

        /* ========== 700ms: 电池状态查询 ========== */
        if (now - ts[6] >= 700) {
            ts[6] = now;
            /* TODO: app_bat_status_process(); */
        }

        /* ========== 睡眠判断 ========== */
        /*
         * 条件: RF保护窗口已过（发送完成 + 等待ACK的时间）
         * 窗口内继续处理接收队列
         */
        if (now >= rf_guard_deadline) {
            app_enter_sleep_with_wakeup_by_timer(20, 1);
        } else {
            process_rx_queue(sched);
        }
    }
}

void RC_Scheduler_SetESCAddr(RC_Scheduler_t *sched, uint8_t *addr)
{
    memcpy(sched->esc_addr, addr, 5);
}

