#ifndef _DRV_GPIO_H_
#define _DRV_GPIO_H_

#include <stdint.h>

//---------------------------------寄存器映射-----------------------------

#define BASEADDR_GPIO           0x00800A00

// 配置寄存器基址 (GPIO 0~39, 每个占4字节)
#define REG_GPIO_CFG_BASE       (BASEADDR_GPIO)

// 计算gpio_num的配置寄存器地址
#define REG_GPIO_CFG(gpio_num)        (*((volatile uint32_t*)(REG_GPIO_CFG_BASE + ((gpio_num) * 4))))

// 中断相关寄存器 (仅支持 GPIO 0~31)
#define REG_GPIO_INT_TYPE_L     *((volatile uint32_t*)(BASEADDR_GPIO + 0x30*4)) // 0~15
#define REG_GPIO_INT_TYPE_H     *((volatile uint32_t*)(BASEADDR_GPIO + 0x31*4)) // 16~31
//原厂代码中 0x33 是 ENABLE, 0x35 是 STATUS
#define REG_GPIO_INT_EN         *((volatile uint32_t*)(BASEADDR_GPIO + 0x33*4)) 
#define REG_GPIO_INT_STAT       *((volatile uint32_t*)(BASEADDR_GPIO + 0x35*4)) // 写1清除

#define GPIO_INPUT_VALUE   0
#define GPIO_OUTPUT_VALUE  1
#define GPIO_INPUT_EN   2
#define GPIO_OUTPUT_EN  3
#define GPIO_PULL_MODE  4
#define GPIO_PULL_EN    5
#define GPIO_2FUN_EN    6
#define GPIO_MONITOR_EN    7



//共有5个port，每个port 8个pin，共40个pin,对应编号0~39
//但是根据寄存器手册和原厂代码 只有0~31号gpio能用，我们的实现也先只支持0~39


typedef enum {
    Port0 = 0,
    Port1 = 1,
    Port2 = 2,
    Port3 = 3,
    Port4 = 4,
} gpio_port_t;

typedef enum {
    Pin0 = 0,
    Pin1 = 1,
    Pin2 = 2,
    Pin3 = 3,
    Pin4 = 4,
    Pin5 = 5,
    Pin6 = 6,
    Pin7 = 7,
} gpio_pin_t;

//port_num:0~39
typedef uint8_t gpio_num_t;

/**
 * @brief  (Port, Pin) -> gpio_num
 * @param  port: GPIO_PORT_0 ~ GPIO_PORT_4
 * @param  pin:  0 ~ 7
 * @return 0 ~ 39
 * @note   应用层传参，gpio_config(Port_Pin(0,3), ...)
 */
#define Port_Pin(port, pin)      ((gpio_num_t)(((port) << 3) + (pin)))

/**
 * @brief  gpio_num -> Port
 * @param  gpio_num: 0 ~ 39
 * @return 0 ~ 4
 */
#define GPIO_GET_PORT(gpio_num)    ((uint8_t)((gpio_num) >> 3))

/**
 * @brief  gpio_num -> Pin
 * @param  gpio_num: 0 ~ 39
 * @return 0 ~ 7
 */
#define GPIO_GET_PIN(gpio_num)     ((uint8_t)((gpio_num) & 0x07))

/**
 * @brief  生成位掩码 (仅适用于 Port 0~3 的中断状态位)
 * @param  gpio_num: 0 ~ 31
 * @return uint32_t mask
 */
#define GPIO_BIT_MASK(gpio_num)    (1UL << (gpio_num))



typedef enum {
    GPIO_INPUT,
    GPIO_OUTPUT,
    GPIO_FLOAT,
    GPIO_SC_FUN, // 第二功能 (Second Function)
} gpio_dir_t;

typedef enum {
    GPIO_PULL_LOW,
    GPIO_PULL_HIGH,
    GPIO_PULL_NONE
} gpio_pull_t;

typedef enum {
    GPIO_INT_LEVEL_LOW    = 0,
    GPIO_INT_LEVEL_HIGH   = 1,
    GPIO_INT_EDGE_RISING  = 2,
    GPIO_INT_EDGE_FALLING = 3,
} gpio_int_mode_t;

typedef void (*gpio_isr_t)(void);

// -----------------------------------------------------------
// 5. 函数接口 (统一使用 gpio_num_t 即 0~39)
// -----------------------------------------------------------
void gpio_init(void);
void gpio_config(gpio_num_t gpio_num, gpio_dir_t dir, gpio_pull_t pull);
void gpio_set(gpio_num_t gpio_num, uint8_t val);
uint8_t gpio_get_input(gpio_num_t gpio_num);
void gpio_toggle(gpio_num_t gpio_num);

// 中断接口 (仅限 gpio_num < 32)
void gpio_int_enable(gpio_num_t gpio_num, gpio_int_mode_t mode, gpio_isr_t cb);
void gpio_int_disable(gpio_num_t gpio_num);
#endif // _GPIO_H_