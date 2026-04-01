#include "app_sleep.h"
#include "icu.h"
#include "aon_rtc.h"
#include "BK3633_RegList.h"
#include "user_config.h"
#include "timer_handler.h"
#include "hal_drv_rf.h"
#include "intc.h"
#include "reg_intc.h"


//调用icu中的休眠函数，进入深度睡眠，并设置aon定时唤醒
/*
*/
void app_enter_sleep_with_wakeup_by_timer(uint32_t sleep_ms,uint8_t sleep_flag)
{
    if(!sleep_flag)
        return ;

    //设置定时器1_1唤醒时间
    uart_printf("[S1]\r\n");
    Timer1_Start_setload_value(1, sleep_ms *2); //timer1_1做唤醒源，cnt单位为0.5ms

    // 清除RF所有中断标志（RF还在上电状态，可以访问寄存器）
    //__HAL_RF_CLEAR_IRQ_FLAGS(IRQ_RX_DR_MASK | IRQ_TX_DS_MASK | IRQ_MAX_RT_MASK);

    // 完全关闭RF模块
    // __HAL_RF_PowerDown();
    // __HAL_RF_CHIP_DIS();

    // // 禁用BK24系统中断，防止唤醒后ISR访问已下电的RF寄存器导致卡死
    clrf_SYS_Reg0x10_int_bk24_en;
    //清除BK24中断挂起位
    intc_status_clear(INT_BK24_bit);
    uart_printf("[S2]:%d\r\n",Get_SysTick_ms());
    cpu_24_reduce_voltage_sleep(); //进入低电压睡眠
    uart_printf("[S3]\r\n");
    // __HAL_RF_PowerUp();
    // __HAL_RF_CHIP_EN();

    // // 恢复BK24系统中断
     setf_SYS_Reg0x10_int_bk24_en;

}
