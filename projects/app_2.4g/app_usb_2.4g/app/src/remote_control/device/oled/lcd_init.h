/**
 ****************************************************************************************
 * @file lcd_init.h
 * @brief LCD/OLED底层驱动接口 - SPI通信和寄存器操作
 *
 * 本文件提供LCD/OLED屏幕的底层驱动函数，包括：
 * - SPI通信封装（硬件SPI + DMA）
 * - 寄存器读写操作
 * - 初始化函数
 * - 基础绘图函数
 *
 * 支持的驱动IC：SH8501A0
 * 支持的屏幕：128x240 AMOLED/OLED
 ****************************************************************************************
 */

#ifndef _LCD_INIT_H_
#define _LCD_INIT_H_

#include "spi.h"
#include "oled_config.h"

/* ============================================================================
 * GPIO控制宏 - 用于软件SPI模式
 * ============================================================================ */
#define LCD_SCK_Clr()  gpio_set(LCD_SCK_GPIO_PIN, 0)   // SPI时钟拉低
#define LCD_SCK_Set()  gpio_set(LCD_SCK_GPIO_PIN, 1)   // SPI时钟拉高

#define LCD_MOSI_Clr() gpio_set(LCD_MOSI_GPIO_PIN, 0)  // SPI数据线拉低
#define LCD_MOSI_Set() gpio_set(LCD_MOSI_GPIO_PIN, 1)  // SPI数据线拉高

#define LCD_RES_Clr()  gpio_set(LCD_RES_GPIO_PIN, 0)   // 复位引脚拉低（触发复位）
#define LCD_RES_Set()  gpio_set(LCD_RES_GPIO_PIN, 1)   // 复位引脚拉高（正常工作）

#define LCD_DC_Clr()   gpio_set(LCD_DC_GPIO_PIN, 0)    // DCX=0：发送命令
#define LCD_DC_Set()   gpio_set(LCD_DC_GPIO_PIN, 1)    // DCX=1：发送数据

#define LCD_CS_Clr()   gpio_set(LCD_CS_GPIO_PIN, 0)    // 片选拉低（选中LCD）
#define LCD_CS_Set()   gpio_set(LCD_CS_GPIO_PIN, 1)    // 片选拉高（释放LCD）

#define LCD_KEY1_Clr() gpio_set(LCD_KEY1_GPIO_PIN, 0)  // 额外控制引脚拉低
#define LCD_KEY1_Set() gpio_set(LCD_KEY1_GPIO_PIN, 1)  // 额外控制引脚拉高

/**
 * @brief LCD数据/命令选择枚举
 *
 * DCX引脚用于区分发送的是命令还是数据：
 * - LCD_TX_CMD (0)：发送命令（寄存器地址）
 * - LCD_TX_DATA (1)：发送数据（参数、像素数据）
 */
typedef enum
{
    LCD_TX_CMD  = 0,  // 命令模式：DCX=0，用于发送寄存器地址
    LCD_TX_DATA = 1,  // 数据模式：DCX=1，用于发送参数或像素数据
} lcd_tx_dc_t;

/**
 * @brief DMA传输完成回调函数类型
 */
typedef void (*lcd_tx_done_cb_t)(void);

/* ============================================================================
 * SPI事务管理函数（用于DMA传输）
 * ============================================================================ */

/**
 * @brief 开始一个SPI事务
 * @param dc 数据/命令选择（LCD_TX_CMD或LCD_TX_DATA）
 *
 * 功能：
 * 1. 设置DCX引脚（命令/数据模式）
 * 2. 拉低CS片选（选中LCD）
 * 3. 准备开始SPI传输
 */
void LCD_TX_Begin(lcd_tx_dc_t dc);

/**
 * @brief 结束一个SPI事务
 *
 * 功能：
 * 1. 等待SPI传输完成
 * 2. 拉高CS片选（释放LCD）
 *
 * 注意：在DMA模式下，CS释放会在DMA完成中断中自动执行
 */
void LCD_TX_End(void);

/**
 * @brief 异步DMA写入（非阻塞）
 * @param buf 数据缓冲区指针
 * @param len 数据长度（字节）
 * @param done_cb 传输完成回调函数（可为NULL）
 *
 * 功能：启动DMA传输，函数立即返回，不等待传输完成
 *
 * 注意：
 * - buf必须是全局变量或静态变量，不能是栈变量
 * - 传输完成后会调用done_cb回调函数
 */
void LCD_TX_Write_DMA_Async(const uint8_t *buf, uint16_t len, lcd_tx_done_cb_t done_cb);

/**
 * @brief 等待DMA传输完成
 *
 * 功能：阻塞等待，直到当前DMA传输完成
 *
 * 注意：在DMA传输期间不要进入深度睡眠
 */
void LCD_TX_WaitDone(void);

/* ============================================================================
 * 底层寄存器操作函数
 * ============================================================================ */

/**
 * @brief 写寄存器地址（命令）
 * @param reg 寄存器地址（8位）
 * 功能：向LCD发送寄存器地址，用于选择要操作的寄存器
 * 示例：
 *   LCD_WR_REG(0x2A);  // 选择列地址寄存器
 */
void LCD_WR_REG(uint8_t reg);

/**
 * @brief 写8位数据
 * @param dat 数据（8位）
 *
 * 功能：向当前选中的寄存器写入8位数据
 *
 * 示例：
 *   LCD_WR_REG(0x2A);      // 选择列地址寄存器
 *   LCD_WR_DATA8(0x00);    // 写入起始列地址高字节
 *   LCD_WR_DATA8(0x00);    // 写入起始列地址低字节
 */
void LCD_WR_DATA8(uint8_t dat);

/**
 * @brief 写16位数据
 * @param dat 数据（16位）
 *
 * 功能：向当前选中的寄存器写入16位数据（自动拆分为2个字节）
 *
 * 示例：
 *   LCD_WR_DATA(0xF800);  // 写入红色像素（RGB565格式）
 */
void LCD_WR_DATA(uint16_t dat);

/**
 * @brief 批量写数据（DMA加速）
 * @param dat 数据缓冲区指针
 * @param size 数据长度（字节）
 *
 * 功能：使用DMA批量写入数据，适合大量像素数据传输
 *
 * 注意：dat必须是全局变量或静态变量
 */
void LCD_WR_DATA_dma(uint8_t* dat, uint16_t size);

/**
 * @brief 批量写数据（分段传输）
 * @param dat 数据缓冲区指针
 * @param size 数据长度（字节）
 *
 * 功能：分段写入数据，适合中等数据量传输
 */
void LCD_WR_DATA_Part(uint8_t* dat, uint16_t size);

/* ============================================================================
 * 底层SPI传输函数（内部使用）
 * ============================================================================ */

/**
 * @brief 发送单字节数据（内部函数）
 * @param dat 数据（8位）
 */
void LCD_WR_Bus(uint8_t dat);

/**
 * @brief 发送16位数据（内部函数）
 * @param dat 数据（16位）
 */
void LCD_WR_Bus_16(uint16_t dat);

/**
 * @brief 发送16位数据（分段，内部函数）
 * @param dat 数据缓冲区指针（2字节）
 */
void LCD_WR_Bus_16_Part(uint8_t* dat);

/**
 * @brief DMA批量发送（内部函数）
 * @param dat 数据缓冲区指针
 * @param size 数据长度（字节）
 */
void LCD_WR_Bus_dma(uint8_t* dat, uint16_t size);

/**
 * @brief 批量发送（内部函数）
 * @param dat 数据缓冲区指针
 * @param size 数据长度（字节）
 */
void LCD_WR_Bus_batch(uint8_t* dat, uint16_t size);

/* ============================================================================
 * 基础绘图函数
 * ============================================================================ */

/**
 * @brief 设置显示区域（窗口）
 * @param xs 起始X坐标
 * @param ys 起始Y坐标
 * @param xe 结束X坐标
 * @param ye 结束Y坐标
 *
 * 功能：设置LCD的显示窗口，后续写入的像素数据会填充到这个区域
 *
 * 示例：
 *   LCD_Address_Set(0, 0, 127, 239);  // 设置全屏窗口
 */
void LCD_Address_Set(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);

/**
 * @brief 填充矩形区域
 * @param xs 起始X坐标
 * @param ys 起始Y坐标
 * @param xe 结束X坐标
 * @param ye 结束Y坐标
 * @param color 填充颜色（RGB565格式）
 *
 * 功能：用指定颜色填充矩形区域
 *
 * 示例：
 *   LCD_Fill(0, 0, 127, 239, BLACK);  // 全屏填充黑色（清屏）
 *   LCD_Fill(10, 10, 50, 50, RED);    // 绘制红色矩形
 */
void LCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);

/**
 * @brief 显示图片（内部函数）
 * @param dat 图片数据指针
 */
void display_image(uint8_t* dat);

/* ============================================================================
 * 初始化函数
 * ============================================================================ */

/**
 * @brief LCD初始化函数（用于TFT LCD）
 *
 * 功能：
 * 1. 配置GPIO引脚（CS、DCX、RST）
 * 2. 执行硬件复位
 * 3. 配置LCD驱动IC寄存器
 * 4. 使能显示输出
 *
 * 注意：
 * - 适用于TFT LCD屏幕
 * - 如果你的屏幕是AMOLED，请使用OLED_Init()
 * - 只需在程序启动时调用一次
 */
void LCD_Init(void);

/**
 * @brief OLED/AMOLED初始化函数（推荐使用）⭐
 *
 * 功能：
 * 1. 配置GPIO引脚（CS、DCX、RST）
 * 2. 执行硬件复位（延时更长，适合AMOLED）
 * 3. 配置SH8501A0驱动IC寄存器
 * 4. 使能显示输出
 *
 * 注意：
 * - 适用于AMOLED/OLED屏幕
 * - 你的屏幕（ZJY095-2424AM002）是AMOLED，应该使用这个函数
 * - 只需在程序启动时调用一次
 *
 * 使用示例：
 *   spi_init(0, 2, 0);              // 初始化SPI
 *   OLED_Init();                    // 初始化OLED
 *   LCD_Fill(0, 0, 127, 239, BLACK); // 清屏
 */
void OLED_Init(void);

/* ============================================================================
 * RGB565颜色定义
 *
 * RGB565格式：16位颜色，5位红色 + 6位绿色 + 5位蓝色
 * 格式：RRRR RGGG GGGB BBBB
 * ============================================================================ */

// 基础颜色
#define WHITE       0xFFFF  // 白色
#define BLACK       0x0000  // 黑色
#define RED         0xF800  // 红色
#define GREEN       0x07E0  // 绿色
#define BLUE        0x001F  // 蓝色

// 混合颜色
#define YELLOW      0xFFE0  // 黄色（红+绿）
#define CYAN        0x7FFF  // 青色（绿+蓝）
#define MAGENTA     0xF81F  // 洋红（红+蓝）

// 特殊颜色
#define BRED        0xF81F  // 亮红
#define GRED        0xFFE0  // 绿红（黄色）
#define GBLUE       0x07FF  // 绿蓝（青色）
#define BROWN       0xBC40  // 棕色
#define BRRED       0xFC07  // 棕红
#define GRAY        0x8430  // 灰色
#define DARKBLUE    0x01CF  // 深蓝
#define LIGHTBLUE   0x7D7C  // 浅蓝
#define GRAYBLUE    0x5458  // 灰蓝
#define LIGHTGREEN  0x841F  // 浅绿
#define LGRAY       0xC618  // 浅灰
#define LBBLUE      0x2B12  // 浅蓝（另一种）
#define LGRAYBLUE   0xA651  // 浅灰蓝
#define MBLUE       0x72FF  // 中蓝
#define MYELLOW     0xF5E4  // 中黄

#endif // _LCD_INIT_H_
