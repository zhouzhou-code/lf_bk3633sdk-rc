/*
 * timer_handler.c
 * 调用driver_timer.c中的定时器相关接口，完成handler的注册与初始化
 *  Created on: 
*/
#include "timer_handler.h"
#include "gpio_init.h"
#include "user_config.h"
#include "drv_gpio.h"


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
void delay_ms(uint32_t ms)
{
    uint32_t start = Get_SysTick_ms();
    while((Get_SysTick_ms() - start) < ms);
}


/* 使用timer1_1绑定pair按键消抖 */
static void pairkey_timercb(void)
{
    static uint8_t check_count = 1;
    if(gpio_get_input(Port_Pin(pair_port,pair_pin))==0)
    {
        uart_printf("pair timer cb:%d\r\n", check_count++);
    }
    //uart_printf("pair timer cb:%d\r\n", check_count++);
    //gpio_int_enable(Port_Pin(pair_port,pair_pin), GPIO_INT_EDGE_FALLING,NULL); //重新使能按键中断
    Timer1_Stop(1);
}
static void timer_pairkey_init(void)
{
    Timer_Initial(1,1,15); // T1_0,1M counter每1us递减一次
    //15ms一次的定时中断
    //Timer1_Start(1,15000); //15ms
    timer_cb_register(1,1,pairkey_timercb);
    //中断使能  需要去intc.c中手动使能timer1中断
}


void Timer_Handler_Init(void)
{
    timer_sys_timebase_init();
    timer_pairkey_init();
}