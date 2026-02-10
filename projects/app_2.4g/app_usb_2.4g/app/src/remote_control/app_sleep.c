#include "app_sleep.h"
#include "icu.h"
#include "aon_rtc.h"
#include "BK3633_RegList.h"

//调用icu中的休眠函数，进入深度睡眠，并设置aon rtc定时唤醒
void app_enter_deep_sleep_with_wakeup_by_rtc(uint32_t sleep_ms)
{
    //设置aon rtc唤醒时间
    setf_rtc_aon_Reg0x0_rtc_cnt_reset; //cnt_reset

    addrtc_aon_Reg0x2 = sleep_ms * 32; //设置唤醒时间，单位为ms，rtc计数器为32kHz
    setf_rtc_aon_Reg0x0_rtc_aon_int; //清除中断标志位
    setf_rtc_aon_Reg0x0_rtc_tick_int_en;
    cpu_24_reduce_voltage_sleep(); //进入低电压睡眠

    // 代码执行到这里，说明已经被中断唤醒，且 ISR 已经执行完毕

    // 立即关闭 Tick 中断
    // 如果不关，系统醒着的时候每隔 10ms 还会被中断打断一次。
    clrf_rtc_aon_Reg0x0_rtc_tick_int_en;

}
