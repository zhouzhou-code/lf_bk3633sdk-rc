#include "app_bat_passthrough.h"

#include <stdint.h>
#include <string.h>

#include "drv_gpio.h"
#include "my_drv_uart.h"
#include "timer_handler.h"
#include "user_config.h"

#include "app_addr_manage.h"
#include "app_key_scan.h"
#include "bat_protocol.h"
#include "rf_handler.h"
#include "rf_pair.h"

#define APP_BAT_LOG uart0_printf

typedef enum {
    APP_MODE_PASSTHROUGH = 0,
    APP_MODE_PAIRING
} app_mode_t;

static void app_bat_pairing_stop(uint8_t *pair_flag)
{
    *pair_flag = 0;
    /* Ensure rf_pair internal last_flag is reset on this round. */
    Slave_Pairing_Task(pair_flag);
}

void app_bat_passthrough_run(void)
{
    const uint32_t pair_total_timeout_ms = 10000;
    uint8_t pair_flag = 0;
    app_mode_t app_mode = APP_MODE_PASSTHROUGH;
    uint8_t last_soc_valid = 0;
    uint8_t last_soc = 0;
    uint8_t pair_rsp_addr = 0;
    uint8_t pair_io_pending = 0;
    uint8_t pair_cmd_pending = 0;
    uint8_t pairing_running = 0;
    uint32_t pair_start_tick = 0;
    uint32_t last_tick_2ms = 0;
    uint32_t last_tick_10ms = 0;
    uint32_t last_tick_100ms = 0;
    uint32_t last_tick_200ms = 0;

    RF_Handler_Init();
    {
        const key_config_t key_cfg[] = {
            {KEY_ID_PAIR, Port_Pin(0, 3), 1000, false},
        };
        app_key_init(key_cfg, sizeof(key_cfg) / sizeof(key_cfg[0]));
    }
    APP_BAT_LOG("Battery passthrough v2 start\r\n");

    while (1) {
        uint32_t now = Get_SysTick_ms();

        if ((now - last_tick_2ms) >= 2) {
            last_tick_2ms = now;
            Protocol_ParseByte(&uart0_rxQueue);
        }

        if ((now - last_tick_10ms) >= 10) {
            Bat_Soc_t soc_pkt;
            uint8_t cmd_addr = 0;

            last_tick_10ms = now;

            app_key_scan(10);
            if (app_key_is_pressed(KEY_ID_PAIR)) {
                if (!pair_io_pending) {
                    pair_io_pending = 1;
                    APP_BAT_LOG("PAIR_IO low\r\n");
                }
            } else {
                pair_io_pending = 0;
            }

            if (bat_protocol_take_pair_cmd(&cmd_addr)) {
                pair_cmd_pending = 1;
                pair_rsp_addr = cmd_addr;
                APP_BAT_LOG("PAIR_CMD received, addr=0x%02X\r\n", pair_rsp_addr);
            }

            if (app_mode == APP_MODE_PASSTHROUGH &&
                pair_io_pending && pair_cmd_pending) {
                pair_flag = 1;
                pairing_running = 1;
                pair_start_tick = now;
                app_mode = APP_MODE_PAIRING;
                pair_io_pending = 0;
                pair_cmd_pending = 0;
                APP_BAT_LOG("Enter pairing mode\r\n");
            }

            if (app_mode == APP_MODE_PAIRING) {
                Slave_Pairing_Task(&pair_flag);

                if (pairing_running && pair_flag == 0) {
                    app_bat_pairing_stop(&pair_flag);
                    bat_protocol_send_pair_resp(pair_rsp_addr, 0x00);
                    APP_BAT_LOG("Pair result=SUCCESS\r\n");
                    pairing_running = 0;
                    app_mode = APP_MODE_PASSTHROUGH;
                    pair_io_pending = 0;
                    pair_cmd_pending = 0;
                } else if (pairing_running && (now - pair_start_tick > pair_total_timeout_ms)) {
                    app_bat_pairing_stop(&pair_flag);
                    bat_protocol_send_pair_resp(pair_rsp_addr, 0x01);
                    APP_BAT_LOG("Pair result=FAIL\r\n");
                    pairing_running = 0;
                    app_mode = APP_MODE_PASSTHROUGH;
                    pair_io_pending = 0;
                    pair_cmd_pending = 0;
                }
            } else if (bat_protocol_take_soc(&soc_pkt)) {
                // 无条件处理：只要解析到SOC数据就打印和发送
                uint8_t dest_addr[5];
                app_addr_get_pipe_addr(0, dest_addr);
                RF_txQueue_Send(dest_addr, (uint8_t *)&soc_pkt, sizeof(Bat_Soc_t));
                APP_BAT_LOG("soc=%d%%\r\n", soc_pkt.soc);

                // 更新记录（保留用于可能的其他用途）
                last_soc = soc_pkt.soc;
                last_soc_valid = 1;
            }
        }

        if ((now - last_tick_100ms) >= 100) {
            last_tick_100ms = now;
            RF_Service_Handler(&hrf);
        }

        if ((now - last_tick_200ms) >= 200) {
            uint8_t *rec_data_p;
            uint8_t len;
            last_tick_200ms = now;

            if (RF_rxQueue_Recv(&rec_data_p, &len, NULL) == 1 &&
                len == sizeof(Bat_Soc_t)) {
                uint16_t crc = crc16_modbus(rec_data_p, len - 2);
                Bat_Soc_t bat_soc_tmp;
                memcpy(&bat_soc_tmp, rec_data_p, sizeof(Bat_Soc_t));

                if (crc == bat_soc_tmp.crc16) {
                    APP_BAT_LOG("rf soc=%d%%\r\n", bat_soc_tmp.soc);
                } else {
                    APP_BAT_LOG("rf soc crc err\r\n");
                }
            }
        }
    }
}

void app_bat_host_run(void)
{
    uint8_t pair_flag = 0;
    uint8_t host_ready = 0;
    uint32_t last_tick_10ms = 0;
    uint32_t last_tick_100ms = 0;
    uint32_t last_tick_200ms = 0;

    RF_Handler_Init();
    {
        const key_config_t key_cfg[] = {
            {KEY_ID_PAIR, Port_Pin(0, 3), 1000, false},
        };
        app_key_init(key_cfg, sizeof(key_cfg) / sizeof(key_cfg[0]));
    }
    APP_BAT_LOG("Battery passthrough host start\r\n");

    while (1) {
        uint32_t now = Get_SysTick_ms();

        if ((now - last_tick_10ms) >= 10) {
            key_event_t evt;
            last_tick_10ms = now;
            app_key_scan(10);
            evt = app_key_get_event(KEY_ID_PAIR);
            if (evt == KEY_EVT_DOWN && pair_flag == 0) {
                pair_flag = 1;
                host_ready = 0;
                APP_BAT_LOG("Host pairing start\r\n");
            }
            if (pair_flag) {
                Host_Pairing_Task(&pair_flag);
                if (pair_flag == 0) {
                    APP_BAT_LOG("Host pairing done\r\n");
                    app_addr_apply_to_rf();
                    HAL_RF_SetRxMode(&hrf);
                    host_ready = 1;
                }
            } else if (!host_ready) {
                app_addr_apply_to_rf();
                HAL_RF_SetRxMode(&hrf);
                host_ready = 1;
            }
        }

        if ((now - last_tick_100ms) >= 100) {
            last_tick_100ms = now;
            RF_Service_Handler(&hrf);
        }

        if ((now - last_tick_200ms) >= 200) {
            uint8_t *rec_data_p;
            uint8_t len;
            last_tick_200ms = now;

            if (RF_rxQueue_Recv(&rec_data_p, &len, NULL) == 1 &&
                len == sizeof(Bat_Soc_t)) {
                uint16_t crc = crc16_modbus(rec_data_p, len - 2);
                Bat_Soc_t bat_soc_tmp;
                memcpy(&bat_soc_tmp, rec_data_p, sizeof(Bat_Soc_t));

                if (crc == bat_soc_tmp.crc16) {
                    APP_BAT_LOG("rf soc=%d%%\r\n", bat_soc_tmp.soc);
                } else {
                    APP_BAT_LOG("rf soc crc err\r\n");
                }
            }
        }
    }
}
