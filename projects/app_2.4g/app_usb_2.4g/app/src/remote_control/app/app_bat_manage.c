/**
 ****************************************************************************************
 * @file app_bat_manage.c
 * @brief Battery management functions
 ****************************************************************************************
 */
#include "app_bat_manage.h"
#include "adc.h"
#include "drv_gpio.h"
#include "pinmap_v1.h"
#include "key_scan.h"
#include "timer_handler.h"
#include "user_config.h"

#define BAT_ADC_CHANNEL  2
static void Delay_us(int num)
{
    int x, y;
    for(y = 0; y < num; y++)
    {
        for(x = 0; x < 5; x++);
    }
}
/**
 * @brief 电源管理芯片使能上电
 */
void app_board_power_on(void) {
    // 配置SYS_PWR_EN为输出
    gpio_config(SYS_PWR_EN, GPIO_OUTPUT, GPIO_PULL_NONE);
    
    // 拉高，锁定电源
    gpio_set(SYS_PWR_EN, 1);

    uart_printf("Power Hold: SYS_PWR_EN = HIGH\r\n");

    // 配置电源按键为输入，下拉，按下=高电平
    gpio_config(KEY_SYS_PWR, GPIO_INPUT, GPIO_PULL_LOW);
}
/**
 * @brief 关机
 */
uint8_t app_board_shutdown(uint8_t shutdown_flag) {
    if(!shutdown_flag) 
        return ;
    gpio_set(SYS_PWR_EN, 0);
    uart_printf("Power Off: SYS_PWR_EN = LOW\r\n");
}

void app_bat_manage_init(void)
{
    gpio_config(ADC_VBAT_PWR_EN, GPIO_OUTPUT, GPIO_PULL_NONE);
    gpio_set(ADC_VBAT_PWR_EN, 0);
}




// ==========================================
// 电池电压 ADC 采样相关硬件参数宏定义
// ==========================================

// 1. ADC 分辨率最大值 (10位ADC填1024，12位填4096)
#define ADC_MAX_RESOLUTION   1024

// 2. ADC 内部参考电压，单位：毫伏 (mV) (根据校准结果填1000)
#define ADC_REF_MV           1000

// 3. 硬件分压网络电阻阻值，单位：千欧 (KΩ)
#define VBAT_R_UP_K          180   // 上拉电阻 (连着电池那一端)
#define VBAT_R_DOWN_K        47    // 下拉电阻 (连着GND那一端)
// 4. (自动计算) 分压系数相关的总阻值
#define VBAT_R_TOTAL_K       (VBAT_R_UP_K + VBAT_R_DOWN_K)

/* * 5. 输入 ADC 原始读数，直接输出真实电池电压 (毫伏)
 * 数学公式: Vbat = (adc_raw * Vref * (R_up + R_down)) / (Resolution * R_down)
 * (强制类型转换为 uint32_t 防止中间乘法计算时数据溢出)
 */
#define CALC_VBAT_MV(adc_raw)  ( ((uint32_t)(adc_raw) * ADC_REF_MV * VBAT_R_TOTAL_K) / \
                                 (ADC_MAX_RESOLUTION * VBAT_R_DOWN_K) )
uint16_t app_bat_manage_read_adc(void)
{
    uint16_t adc_value;

    /* 初始化ADC2 */
    adc_init(BAT_ADC_CHANNEL, 1);
    /* 拉高CONTR_ADC */
    gpio_set(ADC_VBAT_PWR_EN, 1);
    Delay_us(5);

    /* 读取ADC数值 */
    adc_value = adc_get_value(BAT_ADC_CHANNEL, 1);

    /* 拉低CONTR_ADC */
    gpio_set(ADC_VBAT_PWR_EN, 0);
    /* 去初始化ADC2 */
    adc_deinit(BAT_ADC_CHANNEL);
    Delay_us(5);

    return adc_value;
}
