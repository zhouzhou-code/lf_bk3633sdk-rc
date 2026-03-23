/**
 ****************************************************************************************
 * @file main.c
 * @brief RC Battery Transmit Firmware - Main Entry
 * @note 电池透传固件：
 *   - UART接收BMS数据(bat_protocol解析)
 *   - RX模式等待遥控器CMD_BAT_QUERY，通过ACK Payload回复CMD_BAT_STATUS
 *   - 配对触发：IO检测 + 串口配对码 (app_bat_passthrough)
 ****************************************************************************************
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "intc.h"
#include "my_drv_uart.h"
#include "my_drv_uart2.h"
#include "flash.h"
#include "reg_access.h"
#include "boot.h"
#include "dbg.h"
#include "icu.h"
#include "wdt.h"
#include "spi.h"
#include "adc.h"
#include "aon_rtc.h"
#include "rf.h"
#include "drv_gpio.h"
#include "driver_timer.h"
#include "timer_handler.h"
#include "rf_handler.h"
#include "user_config.h"
#include "app_bat_passthrough.h"

#if(USB_DRIVER)
#include "usb.h"
#include "driver_icu.h"
#endif

extern void xvr_reg_initial_24(void);

static void stack_integrity_check(void)
{
    if ((REG_PL_RD(STACK_BASE_UNUSED) != BOOT_PATTERN_UNUSED)) {
        while(1) { uart_printf("Stack_Integrity_Check STACK_BASE_UNUSED fail!\r\n"); }
    }
    if ((REG_PL_RD(STACK_BASE_SVC) != BOOT_PATTERN_SVC)) {
        while(1) { uart_printf("Stack_Integrity_Check STACK_BASE_SVC fail!\r\n"); }
    }
    if ((REG_PL_RD(STACK_BASE_FIQ) != BOOT_PATTERN_FIQ)) {
        while(1) { uart_printf("Stack_Integrity_Check STACK_BASE_FIQ fail!\r\n"); }
    }
    if ((REG_PL_RD(STACK_BASE_IRQ) != BOOT_PATTERN_IRQ)) {
        while(1) { uart_printf("Stack_Integrity_Check STACK_BASE_IRQ fail!\r\n"); }
    }
}

void platform_reset(uint32_t error)
{
    uart_printf("reset error = %x\r\n", error);
    GLOBAL_INT_STOP();
    cpu_reset();
}

int main(void)
{
    /* ========== Hardware Init ========== */
    icu_init();
    intc_init();

    #if(UART_PRINTF_ENABLE)
    //uart2_init(1000000);
    uart_init(115200);
    #endif
    uart_printf("BAT firmware init\r\n");

    xvr_reg_initial();

    Timer_Handler_Init();
    uart_printf("Timer init done:%d\r\n", Get_SysTick_ms());

    flash_init();

    #ifndef __USB_TEST__
    mcu_clk_switch(MCU_CLK_16M);
    #else
    mcu_clk_switch(MCU_CLK_64M);
    #endif

    #if(AON_RTC_DRIVER)
    aon_rtc_init();
    #endif

    GLOBAL_INT_START();

    /* ========== Battery Passthrough Main Loop ========== */
    app_bat_passthrough_run();

    while(1) {
        stack_integrity_check();
    }
}
