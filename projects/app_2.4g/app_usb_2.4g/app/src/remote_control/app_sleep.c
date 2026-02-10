#include "app_sleep.h"
#include "icu.h"
#include "aon_rtc.h"


//调用icu中的休眠函数，进入深度睡眠，并设置aon rtc定时唤醒
void app_deep_sleep_wakeup_by_rtc(uint32_t sleep_ms)
{
    cpu_24_reduce_voltage_sleep(); //进入低电压睡眠
    

}
