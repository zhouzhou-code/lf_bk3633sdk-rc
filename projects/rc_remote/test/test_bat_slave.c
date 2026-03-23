/**
 ****************************************************************************************
 * @file test_bat_slave.c
 * @brief 电池端（Slave）模拟 - 支持配对和业务模式
 *
 * 功能：
 *   1. 配对模式：响应遥控器的配对请求
 *   2. 业务模式：接收查询帧(CMD_BAT_QUERY)，回复电池状态帧（ACK Payload）
 *
 * 模拟数据：温度25°C(偏移值65), SOC递减, 状态交替充电/放电
 ****************************************************************************************
 */

#include "rc_protocol.h"
#include "rf_handler.h"
#include "rf_pair.h"
#include "rf_config.h"
#include "hal_drv_rf.h"
#include "timer_handler.h"
#include "user_config.h"
#include <string.h>
#include "debug.h"
#include "key_scan.h"

extern RF_HandleTypeDef hrf;

/* 配对标志 */
static uint8_t pair_flag = 0;

/* 按键事件处理 */
static void bat_key_event_handler(key_id_t id, key_event_t event)
{
    if (id == KEY_ID_RIGHT && event == KEY_EVT_SHORT_PRESS) {
        static uint8_t right_cnt = 0;
        static uint32_t right_last_time = 0;
        uint32_t now = Get_SysTick_ms();

        if (now - right_last_time > 1000) {
            right_cnt = 0;
        }
        right_cnt++;
        right_last_time = now;

        if (right_cnt >= 5) {
            pair_flag = !pair_flag;
            uart_printf("BAT: Pairing flag toggled: %d\r\n", pair_flag);
            right_cnt = 0;
        }
    }
}

/* 模拟电池状态 */
static bat_status_data_t s_bat_status = {
    .temperature = 65,      /* 25°C (偏移+40) */
    .soc         = 100,     /* 满电 */
    .status      = 0x00,    /* 正常 */
};

/* 模拟电池数据变化 */
static void bat_simulate_update(void)
{
    /* SOC 缓慢递减 */
    if (s_bat_status.soc > 0) {
        s_bat_status.soc--;
    }

    /* 温度小幅波动 (25°C ± 1) */
    static uint8_t temp_dir = 0;
    if (temp_dir) {
        if (s_bat_status.temperature < 66) s_bat_status.temperature++;
        else temp_dir = 0;
    } else {
        if (s_bat_status.temperature > 64) s_bat_status.temperature--;
        else temp_dir = 1;
    }
}

/**
 * @brief 处理业务模式下的接收数据
 */
static void bat_process_business_rx(void)
{
    const uint8_t *rx_data;
    uint8_t rx_len, pipes;

    while (RF_rxQueue_Recv(&rx_data, &rx_len, &pipes)) {
        uint8_t cmd, seq;
        const uint8_t *payload;
        uint8_t plen;

        if (proto_parse(rx_data, rx_len, &cmd, &seq, &payload, &plen) != 0)
            continue;

        switch (cmd) {
        case CMD_BAT_QUERY: {
            /* 查询帧无payload, plen应为0 */
            uart_printf("BAT: RX QUERY seq=%d\r\n", seq);

            /* 更新模拟数据 */
            bat_simulate_update();

            /* 打包电池状态帧作为ACK payload */
            uint8_t ack_buf[32];
            uint8_t ack_len = proto_pack(ack_buf, CMD_BAT_STATUS, seq,
                                         &s_bat_status, sizeof(s_bat_status));
            HAL_RF_Attach_PL2ACK(&hrf, pipes, ack_buf, ack_len);

            uart_printf("BAT: ACK STATUS seq=%d temp=%d soc=%d status=0x%02X\r\n",
                        seq, s_bat_status.temperature - 40,
                        s_bat_status.soc, s_bat_status.status);
            break;
        }
        default:
            break;
        }
    }
}

void test_bat_slave_loop(void)
{
    uart_printf("=== BATTERY (Slave) Mode Start ===\r\n");

    /* 初始化按键 */
    key_config_t test_keys[] = {
        {KEY_ID_LEFT,   Port_Pin(0, 2), 2000, false},
        {KEY_ID_RIGHT,  Port_Pin(3, 1), 1000, false},
    };
    key_init(test_keys, sizeof(test_keys)/sizeof(key_config_t));
    key_register_callback(bat_key_event_handler);

    /* 加载配置 */
    rf_config_load_from_flash();

    /* 初始化RF */
    RF_Handler_Init();

    /* 设置配对参数：电池设备，自带地址模式 */
    uint8_t bat_self_addr[5];
    rf_config_generate_addr(bat_self_addr);
    Slave_Pairing_SetConfig(DEV_TYPE_BATTERY, ADDR_MODE_SLAVE_PROVIDE, bat_self_addr);

    /* 从Flash加载自己的地址（电池地址） */
    uint8_t bat_addr[5];
    if (rf_config_read_device_addr(DEV_TYPE_BATTERY, bat_addr)) {
        HAL_RF_SetRxAddress(&hrf, 0, bat_addr, 5);
        uart_printf("BAT addr loaded: %02X %02X %02X %02X %02X\r\n",
                    bat_addr[0], bat_addr[1], bat_addr[2], bat_addr[3], bat_addr[4]);
    } else {
        uart_printf("BAT not paired, using default addr\r\n");
    }

    /* 切换到RX模式 */
    HAL_RF_SetRxMode(&hrf);
    debug_print_rf_registers();

    uart_printf("BAT ready. Press right key 5 times to pair.\r\n");

    while (1) {
        uint32_t now = Get_SysTick_ms();
        static uint32_t last_key_scan = 0;
        static uint8_t last_pair = 0;

        /* 10ms: 按键扫描 */
        if (now - last_key_scan >= 10) {
            last_key_scan = now;
            key_scan(10);
        }

        /* 配对处理 */
        Slave_Pairing_Task(&pair_flag);

        if (pair_flag) {
            last_pair = 1;
            delay_ms(10);
            continue;
        }

        /* 配对刚结束，同步本地地址 */
        if (last_pair) {
            last_pair = 0;
            if (rf_config_read_device_addr(DEV_TYPE_BATTERY, bat_addr)) {
                HAL_RF_SetRxAddress(&hrf, 0, bat_addr, 5);
                uart_printf("BAT addr: %02X %02X %02X %02X %02X\r\n",
                            bat_addr[0], bat_addr[1], bat_addr[2], bat_addr[3], bat_addr[4]);
            }
            HAL_RF_SetRxMode(&hrf);
        }

        /* 业务模式：处理接收数据 */
        RF_Service_Handler(&hrf);
        bat_process_business_rx();

        delay_ms(2);
    }
}
