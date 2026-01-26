#ifndef _TIMER_HANDLER_H_
#define _TIMER_HANDLER_H_

#include "driver_timer.h"

//所有和Timer相关的初始化，除了中断
void Timer_Handler_Init(void);


//handler服务:获取系统运行时间，单位ms
uint32_t Get_SysTick_ms(void);

#endif /* _TIMER_HANDLER_H_ */

