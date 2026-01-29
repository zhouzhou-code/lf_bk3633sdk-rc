#include "drv_gpio.h"
#include "gpio_init.h"
#include "user_config.h"

static volatile uint32_t last_tick = 0;
uint16_t cnt=0;
void key_isr(void) {
    uint32_t now = Get_SysTick_ms();
    if(Get_SysTick_ms() - last_tick < 50) //20ms消抖
    {
        return;
    }
    if (gpio_get_input(Port_Pin(1,0)) == 1) {
        return; 
    }
        
    last_tick = now;
    //按键有效,处理事件
    cnt++;
}


// 假设按键接在GPIO 0x10
void my_key_init(void) {
    // 初始化GPIO和中断
    gpio_config(Port_Pin(1,0), GPIO_INPUT, GPIO_PULL_HIGH); // 输入，上拉
    gpio_int_enable(Port_Pin(1,0), GPIO_INT_EDGE_FALLING,key_isr); // 设置为下降沿中断
    uart_printf("Key Initialized on GPIO 0x10\r\n"); 
}
