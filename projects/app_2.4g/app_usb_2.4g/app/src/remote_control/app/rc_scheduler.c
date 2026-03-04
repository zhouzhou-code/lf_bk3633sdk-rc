#include "rc_scheduler.h"
#include "rf_handler.h"
#include "rf_protocol.h"
#include "timer_handler.h"
#include "key_scan.h"
#include "my_queue.h"
#include <string.h>

__attribute__((weak)) void app_hall_update(void);
__attribute__((weak)) uint16_t app_hall_get_throttle(void);

// 外部队列引用
extern my_queue_t rf_txQueue;
extern my_queue_t rf_rxQueue;
extern RF_HandleTypeDef hrf;

// 初始化RC调度�?
void RC_Scheduler_Init(RC_Scheduler_t *sched)
{
    memset(sched, 0, sizeof(RC_Scheduler_t));

    // 默认配置
    sched->esc_period_ms = 50;   // 电控50ms周期
    sched->bat_period_ms = 700;  // 电池700ms周期
}

// 配置设备地址
void RC_Scheduler_SetESCAddr(RC_Scheduler_t *sched, uint8_t *addr)
{
    memcpy(sched->esc_addr, addr, 5);
}

void RC_Scheduler_SetBATAddr(RC_Scheduler_t *sched, uint8_t *addr)
{
    memcpy(sched->bat_addr, addr, 5);
}

// 配置任务周期
void RC_Scheduler_SetESCPeriod(RC_Scheduler_t *sched, uint16_t period_ms)
{
    sched->esc_period_ms = period_ms;
}

void RC_Scheduler_SetBATPeriod(RC_Scheduler_t *sched, uint16_t period_ms)
{
    sched->bat_period_ms = period_ms;
}

// 发送电控命�?
void RC_Scheduler_SendESCCmd(RC_Scheduler_t *sched, uint16_t throttle)
{
    uint8_t cmd_buf[32];

    // 构造电控命令包
    RF_BuildESCCmd(cmd_buf, throttle, sched->tx_seq++);

    // 添加到发送队�?
    RF_txQueue_Send(sched->esc_addr, cmd_buf, sizeof(ESC_Pkt_t));

    // 记录发送时间和超时
    sched->last_send_time = Get_SysTick_ms();
    sched->rx_timeout_ms = 10;  // 等待10ms
}

// 发送电池查�?
void RC_Scheduler_SendBATQuery(RC_Scheduler_t *sched)
{
    uint8_t query_buf[32];

    // 构造电池查询包
    RF_BuildBATQuery(query_buf, sched->tx_seq++);

    // 添加到发送队�?
    RF_txQueue_Send(sched->bat_addr, query_buf, sizeof(BAT_Pkt_t));

    // 记录发送时间和超时
    sched->last_send_time = Get_SysTick_ms();
    sched->rx_timeout_ms = 20;  // 等待20ms
}

// 处理接收数据
void RC_Scheduler_ProcessRxData(RC_Scheduler_t *sched)
{
    uint8_t *rec_data;
    uint8_t len, pipes;

    // 从接收队列取出数�?
    if(RF_rxQueue_Recv(&rec_data, &len, &pipes) == 1) {
        // 尝试解析电控响应
        ESC_Pkt_t esc_resp;
        if(RF_ParseESCResp(rec_data, len, &esc_resp) == 0) {
            // 成功解析电控响应
            uint16_t speed = esc_resp.data;
            // TODO: 更新速度显示
            // uart_printf("ESC speed: %d\r\n", speed);
        }

        // 尝试解析电池响应
        BAT_Pkt_t bat_resp;
        if(RF_ParseBATResp(rec_data, len, &bat_resp) == 0) {
            // 成功解析电池响应
            uint8_t soc = bat_resp.data;
            // TODO: 更新电池显示
            // uart_printf("Battery SOC: %d%%\r\n", soc);
        }

        // 收到回复，清除等待标�?
        sched->rx_timeout_ms = 0;
    }
}

// RC调度器主任务
void RC_Scheduler_Task(RC_Scheduler_t *sched)
{
    uint32_t now = Get_SysTick_ms();
    static uint32_t last_key_time = 0;

    // 处理RF收发
    RF_Service_Handler(&hrf);

    // 处理接收数据
    RC_Scheduler_ProcessRxData(sched);

    if ((uint32_t)(now - last_key_time) >= 10U) {
        app_key_scan(10);
        last_key_time = now;
    }

    if (sched->rx_timeout_ms == 0) {
        if (sched->esc_period_ms != 0U &&
            (uint32_t)(now - sched->last_esc_time) >= sched->esc_period_ms) {
            if (app_hall_update != 0) {
                app_hall_update();
            }

            uint16_t throttle = 0;
            if (app_hall_get_throttle != 0) {
                throttle = app_hall_get_throttle();
            }

            RC_Scheduler_SendESCCmd(sched, throttle);
            sched->last_esc_time = now;
        }

        if (sched->bat_period_ms != 0U &&
            (uint32_t)(now - sched->last_bat_time) >= sched->bat_period_ms) {
            RC_Scheduler_SendBATQuery(sched);
            sched->last_bat_time = now;
        }
    }
}

// 检查是否可以进入sleep
uint8_t RC_Scheduler_CanSleep(RC_Scheduler_t *sched)
{
    // 如果正在等待响应
    if(sched->rx_timeout_ms > 0) {
        uint32_t elapsed = Get_SysTick_ms() - sched->last_send_time;

        // 如果还没超时且没收到回复，不能sleep
        if(elapsed < sched->rx_timeout_ms && queue_is_empty(&rf_rxQueue)) {
            return 0;  // 不能sleep
        }

        // 超时或收到回复，清除等待标志
        sched->rx_timeout_ms = 0;
    }

    // 检查发送队列是否为�?
    if(!queue_is_empty(&rf_txQueue)) {
        return 0;  // 发送队列不为空，不能sleep
    }

    // TODO: 添加其他sleep条件检�?
    // 例如�?
    // - 按键是否有操�?
    // - 霍尔是否稳定
    // - 其他业务逻辑

    return 1;  // 可以sleep
}

