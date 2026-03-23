/**
 ****************************************************************************************
 * @file app_bat_manage.c
 * @brief 电池管理模块实现
 ****************************************************************************************
 */
#include "bat_manage.h"
#include "adc.h"
#include "drv_gpio.h"
#include "pinmap.h"
#include "timer_handler.h"
#include "user_config.h"

/* ============================================================================
 * 内部常量
 * ============================================================================ */

// ADC参数
#define ADC_MAX_RESOLUTION   1024
#define ADC_REF_MV           1050

/* ============================================================================
 * 内部工具
 * ============================================================================ */

static void delay_us(int num)
{
    int x, y;
    for (y = 0; y < num; y++)
        for (x = 0; x < 5; x++);
}

/**
 * @brief 电压转SOC（线性映射）
 */
static uint8_t voltage_to_soc(uint16_t mv, uint16_t empty_mv, uint16_t full_mv)
{
    if (mv <= empty_mv) return 0;
    if (mv >= full_mv)  return 100;
    return (uint8_t)(((uint32_t)(mv - empty_mv) * 100) / (full_mv - empty_mv));
}

/**
 * @brief ADC原始值转电池电压(mV)
 * V_adc = V_bat * ratio  →  V_bat = V_adc / ratio
 */
static uint16_t adc_to_voltage(uint16_t adc_raw, float ratio)
{
    float v_adc = (float)adc_raw * ADC_REF_MV / ADC_MAX_RESOLUTION;
    return (uint16_t)(v_adc / ratio);
}

/**
 * @brief 读取ADC（使能分压→采样→关闭分压）
 */
static uint16_t read_bat_adc(const bat_hw_config_t *hw)
{
    uint16_t val;

    adc_init(hw->adc_channel, 1);
    gpio_set(hw->adc_pwr_en_gpio, 1);
    delay_us(10);

    val = adc_get_value(hw->adc_channel, 1);

    gpio_set(hw->adc_pwr_en_gpio, 0);
    adc_deinit(hw->adc_channel);

    return val;
}

/**
 * @brief 读取BQ24040充电状态（PGOOD + CHRG组合）
 */
static bat_chg_state_t read_chg_state(const bat_hw_config_t *hw)
{
    uint8_t pgood = gpio_get_input(hw->pgood_gpio);  // 低=电源好
    uint8_t chrg  = gpio_get_input(hw->chrg_gpio);    // 低=充电中

    if (pgood) {
        // PGOOD=HIGH: 无外部电源
        return chrg ? CHG_STATE_NO_POWER : CHG_STATE_FAULT;
    } else {
        // PGOOD=LOW: 有外部电源
        return chrg ? CHG_STATE_FULL : CHG_STATE_CHARGING;
    }
}

/* ============================================================================
 * API 实现
 * ============================================================================ */

void bat_manage_init(bat_manager_t *bat, const bat_hw_config_t *hw)
{
    bat->hw = *hw;

    // 电源控制引脚
    gpio_config(hw->pwr_en_gpio, GPIO_OUTPUT, GPIO_PULL_NONE);
    gpio_config(hw->pwr_key_gpio, GPIO_INPUT, GPIO_PULL_LOW);

    // ADC分压使能引脚（默认关闭，采样时开启）
    gpio_config(hw->adc_pwr_en_gpio, GPIO_OUTPUT, GPIO_PULL_NONE);
    gpio_set(hw->adc_pwr_en_gpio, 0);

    // 充电检测引脚（输入上拉，BQ24040为开漏输出）
    gpio_config(hw->pgood_gpio, GPIO_INPUT, GPIO_PULL_HIGH);
    gpio_config(hw->chrg_gpio, GPIO_INPUT, GPIO_PULL_HIGH);

    // 清零数据
    bat->data.adc_raw    = 0;
    bat->data.voltage_mv = 0;
    bat->data.soc        = 0;
    bat->data.chg_state  = CHG_STATE_NO_POWER;

    bat->initialized = 1;
    uart_printf("bat_manage_init done\r\n");
}

void bat_manage_update(bat_manager_t *bat)
{
    if (!bat->initialized) return;

    // 1. ADC采样 → 电压 → SOC
    bat->data.adc_raw    = read_bat_adc(&bat->hw);
    bat->data.voltage_mv = adc_to_voltage(bat->data.adc_raw, bat->hw.divider_ratio);
    bat->data.soc        = voltage_to_soc(bat->data.voltage_mv,
                                          bat->hw.bat_empty_mv,
                                          bat->hw.bat_full_mv);

    // 2. 充电状态
    bat->data.chg_state  = read_chg_state(&bat->hw);
}

void bat_manage_power_on(bat_manager_t *bat)
{
    gpio_set(bat->hw.pwr_en_gpio, 1);
    uart_printf("Power ON\r\n");
}

void bat_manage_power_off(bat_manager_t *bat)
{
    uart_printf("Power OFF\r\n");
    gpio_set(bat->hw.pwr_en_gpio, 0);
}
