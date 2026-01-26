/*
 * timer_handler.c
 * 调用driver_timer.c中的定时器相关接口，完成handler的注册与初始化
 *  Created on: 
*/
#include "timer_handler.h"


/* 使用timer1_0作为系统ms级时基 */
static volatile uint32_t Systick_ms=0; 
static void Inc_Systick(void)
{
    Systick_ms++;
}
static void timer_sys_timebase_init(void)
{
    Timer_Initial(1,1,15); // T1_0,1M counter每1us递减一次
    //1ms一次的定时中断
    Timer1_Start(0,1000); // 1ms
    timer_cb_register(1,0,Inc_Systick);
    //中断使能  需要去intc.c中手动使能timer1中断
}

uint32_t Get_SysTick_ms(void)
{
    return Systick_ms;
}


void Timer_Handler_Init(void)
{
    timer_sys_timebase_init();
}