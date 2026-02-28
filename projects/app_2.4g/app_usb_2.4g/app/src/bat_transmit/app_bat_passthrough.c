#include "app_bat_passthrough.h"

#include <stdint.h>

#include "drv_gpio.h"
#include "my_drv_uart.h"
#include "timer_handler.h"
#include "user_config.h"

#include "app_addr_manage.h"
#include "bat_protocol.h"
#include "rf_handler.h"
#include "rf_pair.h"

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
    const uint16_t pair_io_debounce_ms = 20;
    uint8_t pair_flag = 0;
    app_mode_t app_mode = APP_MODE_PASSTHROUGH;
    uint8_t last_soc_valid = 0;
    uint8_t last_soc = 0;
    uint8_t pair_rsp_addr = 0;
    uint8_t pair_io_pending = 0;
    uint8_t pair_io_latched = 0;
    uint8_t pair_cmd_pending = 0;
    uint8_t pairing_running = 0;
    uint16_t pair_io_low_acc_ms = 0;
    uint32_t pair_start_tick = 0;
    uint32_t last_tick_2ms = 0;
    uint32_t last_tick_10ms = 0;
    uint32_t last_tick_100ms = 0;

    RF_Handler_Init();
    gpio_config(Port_Pin(0, 3), GPIO_INPUT, GPIO_PULL_HIGH);
    uart_printf("Battery passthrough v2 start\r\n");

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

            if (gpio_get_input(Port_Pin(0, 3)) == 0) {
                if (pair_io_low_acc_ms < pair_io_debounce_ms) {
                    pair_io_low_acc_ms += 10;
                }
                if (pair_io_low_acc_ms >= pair_io_debounce_ms && !pair_io_latched) {
                    pair_io_pending = 1;
                    pair_io_latched = 1;
                    uart_printf("PAIR_IO low\r\n");
                }
            } else {
                pair_io_low_acc_ms = 0;
                pair_io_latched = 0;
                pair_io_pending = 0;
            }

            if (bat_protocol_take_pair_cmd(&cmd_addr)) {
                pair_cmd_pending = 1;
                pair_rsp_addr = cmd_addr;
                uart_printf("PAIR_CMD received, addr=0x%02X\r\n", pair_rsp_addr);
            }

            if (app_mode == APP_MODE_PASSTHROUGH &&
                pair_io_pending && pair_cmd_pending) {
                pair_flag = 1;
                pairing_running = 1;
                pair_start_tick = now;
                app_mode = APP_MODE_PAIRING;
                pair_io_pending = 0;
                pair_cmd_pending = 0;
                uart_printf("Enter pairing mode\r\n");
            }

            if (app_mode == APP_MODE_PAIRING) {
                Slave_Pairing_Task(&pair_flag);

                if (pairing_running && pair_flag == 0) {
                    app_bat_pairing_stop(&pair_flag);
                    bat_protocol_send_pair_resp(pair_rsp_addr, 0x00);
                    uart_printf("Pair result=SUCCESS\r\n");
                    pairing_running = 0;
                    app_mode = APP_MODE_PASSTHROUGH;
                    pair_io_pending = 0;
                    pair_cmd_pending = 0;
                    pair_io_low_acc_ms = 0;
                } else if (pairing_running && (now - pair_start_tick > pair_total_timeout_ms)) {
                    app_bat_pairing_stop(&pair_flag);
                    bat_protocol_send_pair_resp(pair_rsp_addr, 0x01);
                    uart_printf("Pair result=FAIL\r\n");
                    pairing_running = 0;
                    app_mode = APP_MODE_PASSTHROUGH;
                    pair_io_pending = 0;
                    pair_cmd_pending = 0;
                    pair_io_low_acc_ms = 0;
                }
            } else if (bat_protocol_take_soc(&soc_pkt)) {
                if (!last_soc_valid || soc_pkt.soc != last_soc) {
                    uint8_t dest_addr[5];
                    app_addr_get_pipe_addr(0, dest_addr);
                    RF_txQueue_Send(dest_addr, (uint8_t *)&soc_pkt, sizeof(Bat_Soc_t));
                    last_soc = soc_pkt.soc;
                    last_soc_valid = 1;
                    uart_printf("soc=%d%%\r\n", soc_pkt.soc);
                }
            }
        }

        if ((now - last_tick_100ms) >= 100) {
            last_tick_100ms = now;
            RF_Service_Handler(&hrf);
        }
    }
}
