/**
 ****************************************************************************************
 * @file pinmap_v1.h
 * @brief Pin mapping definition for REMOTE-CONTROL hardware v1.0
 * @note Based on BK3633 chip, refer to REMOTE-CONTROL0303.pdf schematic
 ****************************************************************************************
 */
#ifndef _PINMAP_V1_H_
#define _PINMAP_V1_H_

#include "drv_gpio.h"

/* ============================================================================
 * 按键输入模块 (Keys Input Module)
 * 配置：GPIO输入模式，内部上拉，按下时读到低电平
 * ============================================================================ */
#define KEY_LEFT        Port_Pin(0, 0)  // P00 - 左键
#define KEY_RIGHT       Port_Pin(0, 1)  // P01 - 右键
#define KEY_MIDDLE      Port_Pin(0, 2)  // P02 - 中键/滚轮按键
#define KEY_PAIR        Port_Pin(3, 1)  // P31 - 配对键

/* ============================================================================
 * LED指示灯模块 (LED Indicator Module)
 * 配置：GPIO输出模式，高电平点亮
 * ============================================================================ */
#define LED1            Port_Pin(0, 3)  // P03 - 指示灯1
#define LED2            Port_Pin(0, 4)  // P04 - 指示灯2

/* ============================================================================
 * 调试串口模块 (Debug UART Module)
 * 协议：UART0，波特率115200
 * ============================================================================ */
#define UART0_TX        Port_Pin(0, 5)  // P05 - 串口0发送
#define UART0_RX        Port_Pin(0, 6)  // P06 - 串口0接收

/* ============================================================================
 * 霍尔传感器模块 (Hall Sensor Module)
 * 协议：I2C，地址根据具体传感器型号确定
 * 常见型号：MLX90393, TLV493D, AK09918等磁力计
 * ============================================================================ */
#define HALL_INT        Port_Pin(1, 0)  // P10 - 传感器中断输入
#define HALL_SDA        Port_Pin(1, 1)  // P11 - I2C数据线
#define HALL_SCL        Port_Pin(1, 2)  // P12 - I2C时钟线

/* ============================================================================
 * 电池电压检测模块 (Battery Voltage Monitor Module)
 * 配置：ADC输入，通过分压电阻检测电池电压
 * ============================================================================ */
#define ADC_VBAT        Port_Pin(1, 3)  // P13 - 电池电压检测

/* ============================================================================
 * LCD显示屏模块 (LCD Display Module)
 * 协议：SPI (4线SPI + DC/RST控制)
 * 支持：OLED/TFT小屏幕，如SSD1306, ST7735等
 * ============================================================================ */
#define LCD_SPI_CLK     Port_Pin(2, 0)  // P20 - SPI时钟
#define LCD_SPI_MOSI    Port_Pin(2, 1)  // P21 - SPI主机输出
#define LCD_SPI_CS      Port_Pin(2, 2)  // P22 - SPI片选
#define LCD_DC          Port_Pin(2, 3)  // P23 - 数据/命令选择
#define LCD_RST         Port_Pin(2, 4)  // P24 - LCD复位

/* ============================================================================
 * 保留/未使用引脚 (Reserved/Unused Pins)
 * 这些引脚在当前硬件版本中未使用，可用于未来扩展
 * ============================================================================ */
// Port 0: P07
// Port 1: P14, P15, P16, P17
// Port 2: P25, P26, P27
// Port 3: P30, P32, P33, P34

/* ============================================================================
 * 引脚初始化宏 (Pin Initialization Macros)
 * ============================================================================ */

/**
 * @brief 初始化所有按键引脚
 */
#define PINMAP_INIT_KEYS() \
    do { \
        gpio_config(KEY_LEFT,   GPIO_INPUT, GPIO_PULL_HIGH); \
        gpio_config(KEY_RIGHT,  GPIO_INPUT, GPIO_PULL_HIGH); \
        gpio_config(KEY_MIDDLE, GPIO_INPUT, GPIO_PULL_HIGH); \
        gpio_config(KEY_PAIR,   GPIO_INPUT, GPIO_PULL_HIGH); \
    } while(0)

/**
 * @brief 初始化所有LED引脚
 */
#define PINMAP_INIT_LEDS() \
    do { \
        gpio_config(LED1, GPIO_OUTPUT, GPIO_PULL_NONE); \
        gpio_config(LED2, GPIO_OUTPUT, GPIO_PULL_NONE); \
        gpio_set(LED1, 0); \
        gpio_set(LED2, 0); \
    } while(0)

/**
 * @brief 初始化UART0调试引脚
 */
#define PINMAP_INIT_UART0() \
    do { \
        gpio_config(UART0_TX, GPIO_SC_FUN, GPIO_PULL_HIGH); \
        gpio_config(UART0_RX, GPIO_SC_FUN, GPIO_PULL_HIGH); \
    } while(0)

/**
 * @brief 初始化霍尔传感器引脚
 */
#define PINMAP_INIT_HALL() \
    do { \
        gpio_config(HALL_INT, GPIO_INPUT, GPIO_PULL_HIGH); \
        gpio_config(HALL_SDA, GPIO_SC_FUN, GPIO_PULL_HIGH); \
        gpio_config(HALL_SCL, GPIO_SC_FUN, GPIO_PULL_HIGH); \
    } while(0)

/**
 * @brief 初始化LCD SPI引脚
 */
#define PINMAP_INIT_LCD() \
    do { \
        gpio_config(LCD_SPI_CLK,  GPIO_SC_FUN, GPIO_PULL_NONE); \
        gpio_config(LCD_SPI_MOSI, GPIO_SC_FUN, GPIO_PULL_NONE); \
        gpio_config(LCD_SPI_CS,   GPIO_OUTPUT, GPIO_PULL_NONE); \
        gpio_config(LCD_DC,       GPIO_OUTPUT, GPIO_PULL_NONE); \
        gpio_config(LCD_RST,      GPIO_OUTPUT, GPIO_PULL_NONE); \
        gpio_set(LCD_SPI_CS, 1);  /* CS默认高电平 */ \
        gpio_set(LCD_RST, 1);     /* RST默认高电平 */ \
    } while(0)

/**
 * @brief 初始化ADC电池检测引脚
 */
#define PINMAP_INIT_ADC_VBAT() \
    do { \
        gpio_config(ADC_VBAT, GPIO_INPUT, GPIO_PULL_NONE); \
    } while(0)

/* ============================================================================
 * 硬件版本信息
 * ============================================================================ */
#define HARDWARE_VERSION_MAJOR  1
#define HARDWARE_VERSION_MINOR  0
#define HARDWARE_VERSION_STRING "v1.0"
#define HARDWARE_PCB_NAME       "REMOTE-CONTROL0303"

#endif // _PINMAP_V1_H_
