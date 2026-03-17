/**
 ****************************************************************************************
 * @file test_slave.c
 * @brief 模拟从机（电控）测试代码
 *
 * 从机流程:
 *   1. RX模式等待遥控帧
 *   2. 收到后解析控制帧，提取seq和油门等数据
 *   3. 打包状态帧(speed/mileage)，带上ack_seq
 *   4. 通过HAL_RF_Attach_PL2ACK装入ACK payload
 *   5. 下次遥控发帧时，硬件自动回传ACK payload
 *
 * 使用方法: 在main.c中调用 test_slave_loop()
 ****************************************************************************************
 */

#include "rc_protocol.h"
#include "rf_handler.h"
#include "hal_drv_rf.h"
#include "timer_handler.h"
#include "user_config.h"
#include <string.h>
#include "debug.h"

void test_slave_loop(void)
{
    uart_printf("=== Slave Mode Start ===\r\n");

    /* 切换到RX模式 */
    RF_Handler_Init();
    HAL_RF_SetRxMode(&hrf);
    debug_print_rf_registers();
    /* 模拟电控状态 */
    mc_status_t mc = {
        .speed   = 0,
        .mileage = 0
    };

    uint8_t last_ack_seq = 0;

    while (1) {
        const uint8_t *rx_data;
        uint8_t rx_len, pipes;

        /* 从RX队列取数据 (中断回调已入队) */
        if (RF_rxQueue_Recv(&rx_data, &rx_len, &pipes)) {

            uint8_t seq;
            rc_ctrl_t ctrl;

            /* 解析遥控下行控制帧 */
            if (proto_parse_ctrl(rx_data, rx_len, &seq, &ctrl) == 0) {

                uart_printf("RX seq=%d gear=%d mode=%d throttle=%d\r\n",
                            seq, ctrl.gear, ctrl.mode, ctrl.throttle);

                /* 模拟电控: 根据油门更新速度 */
                mc.speed = ctrl.throttle * 10;
                mc.mileage += 1;

                /* 记录ack_seq */
                last_ack_seq = seq;

                /* 打包状态帧，装入ACK payload */
                uint8_t ack_buf[32];
                uint8_t ack_len = proto_pack_status(ack_buf, last_ack_seq, &mc);
                HAL_RF_Attach_PL2ACK(&hrf, pipes, ack_buf, ack_len);

                uart_printf("ACK seq=%d speed=%d mileage=%lu\r\n",
                            last_ack_seq, mc.speed, mc.mileage);
            }
        }
    }
}
