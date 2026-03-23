/**
 ****************************************************************************************
 * @file bat_manage.h
 * @brief 电池管理模块（电压检测/SOC/充电状态/开关机）
 * @note 参考 hall_sensor 面向对象风格
 ****************************************************************************************
 */
#ifndef __APP_BAT_MANAGE_H__
#define __APP_BAT_MANAGE_H__

#include <stdint.h>

/* ============================================================================
 * 充电状态枚举（BQ24040: PGOOD + CHRG 两脚组合）
 * ============================================================================
 *  PGOOD  CHRG   含义
 *   LOW    LOW   充电中（有电源且正在充电）
 *   LOW    HIGH  充满（有电源，充电完成）
 *   HIGH   HIGH  无电源（未插入USB）
 *   HIGH   LOW   故障（理论上不应出现）
 * ============================================================================ */
typedef enum {
    CHG_STATE_NO_POWER = 0,   // 无外部电源
    CHG_STATE_CHARGING,       // 充电中
    CHG_STATE_FULL,           // 充满
    CHG_STATE_FAULT,          // 故障
} bat_chg_state_t;

/* ============================================================================
 * 硬件配置（引脚 + ADC参数）
 * ============================================================================ */
typedef struct {
    /* 电源控制 */
    uint8_t  pwr_en_gpio;         // SYS_PWR_EN 引脚
    uint8_t  pwr_key_gpio;        // 电源按键输入引脚

    /* 电池ADC */
    uint8_t  adc_channel;         // ADC通道
    uint8_t  adc_pwr_en_gpio;     // ADC分压电阻使能引脚

    /* 充电检测（BQ24040） */
    uint8_t  pgood_gpio;          // PGOOD引脚（低=电源好）
    uint8_t  chrg_gpio;           // CHRG引脚（低=充电中）

    /* 分压系数: V_adc = V_bat * divider_ratio
     * 例: R_up=180K, R_down=47K → ratio = 47/(180+47) = 0.207 */
    float    divider_ratio;

    /* 电池电压范围 (mV)，用于计算SOC */
    uint16_t bat_empty_mv;        // 空电电压 (如 3000)
    uint16_t bat_full_mv;         // 满电电压 (如 4200)
} bat_hw_config_t;

/* ============================================================================
 * 电池状态数据
 * ============================================================================ */
typedef struct {
    uint16_t         adc_raw;     // ADC原始值
    uint16_t         voltage_mv;  // 电池电压 (mV)
    uint8_t          soc;         // 电量百分比 0-100
    bat_chg_state_t  chg_state;   // 充电状态
} bat_data_t;

/* ============================================================================
 * 电池管理对象
 * ============================================================================ */
typedef struct {
    bat_hw_config_t  hw;          // 硬件配置
    bat_data_t       data;        // 状态数据
    uint8_t          initialized; // 初始化标志
} bat_manager_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * @brief 初始化电池管理模块
 * @param bat  电池管理对象
 * @param hw   硬件配置
 */
void bat_manage_init(bat_manager_t *bat, const bat_hw_config_t *hw);

/**
 * @brief 更新电池状态（ADC采样 + 充电检测），周期调用
 * @param bat  电池管理对象
 */
void bat_manage_update(bat_manager_t *bat);

/**
 * @brief 锁定电源（上电后调用，保持系统供电）
 * @param bat  电池管理对象
 */
void bat_manage_power_on(bat_manager_t *bat);

/**
 * @brief 关机（释放电源使能）
 * @param bat  电池管理对象
 */
void bat_manage_power_off(bat_manager_t *bat);

#endif // __APP_BAT_MANAGE_H__
