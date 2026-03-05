# LCD驱动API使用说明

## 一、屏幕硬件信息

**屏幕型号**: 中景园 ZJY095-2424AM002
**尺寸**: 0.95寸
**分辨率**: 128x240
**显示技术**: AMOLED
**驱动IC**: SH8501A0
**通信接口**: SPI (4线SPI + DCX控制)

## 二、目录结构说明

```
app/src/remote_control/device/oled/
├── oled_config.h          # 配置文件：引脚映射、屏幕尺寸、DMA设置
├── lcd_init.h/c           # 底层驱动：SPI通信、寄存器操作、初始化
├── lcd_kmg.h/c            # 上层API：显示文字、数字、图片
├── lcdfont_controller.h   # 字库数据
├── pic.h                  # 图片数据
└── LCD_SPI_CS_TIMING_FIX.md  # SPI时序问题修复文档
```

## 三、函数分类与用途

### 3.1 初始化函数（必须先调用）

#### `LCD_Init()` - TFT LCD初始化
- **用途**: 初始化TFT彩屏（如ST7735、ST7789等）
- **驱动IC**: 通用TFT驱动IC
- **何时使用**: 如果你的屏幕是TFT LCD

#### `OLED_Init()` - AMOLED/OLED初始化  ⭐ **你应该用这个**
- **用途**: 初始化AMOLED/OLED屏幕
- **驱动IC**: SH8501A0（你的屏幕）
- **何时使用**: 你的ZJY095-2424AM002是AMOLED屏，应该使用这个函数

**结论**: 你应该使用 `OLED_Init()`，因为你的屏幕是AMOLED技术。

---

### 3.2 底层通信函数（一般不直接调用）

这些函数封装了SPI通信细节，通常由上层API内部调用：

```c
// 写寄存器命令
void LCD_WR_REG(uint8_t reg);

// 写8位数据
void LCD_WR_DATA8(uint8_t dat);

// 写16位数据
void LCD_WR_DATA(uint16_t dat);

// DMA批量写数据（高速）
void LCD_WR_DATA_dma(uint8_t* dat, uint16_t size);

// 设置显示区域
void LCD_Address_Set(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
```

---

### 3.3 基础绘图函数

#### `LCD_Fill()` - 填充矩形区域
```c
void LCD_Fill(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color);
```
- **参数**:
  - `xs, ys`: 起始坐标
  - `xe, ye`: 结束坐标
  - `color`: RGB565颜色值（如WHITE、BLACK、RED等）
- **用途**: 清屏、绘制矩形色块
- **示例**:
```c
LCD_Fill(0, 0, 127, 239, BLACK);  // 全屏填充黑色（清屏）
LCD_Fill(10, 10, 50, 50, RED);    // 绘制红色矩形
```

---

### 3.4 文字显示函数

#### `LCD_ShowChar()` / `LCD_ShowChar_Hor()` - 显示单个字符
```c
void LCD_ShowChar_Hor(uint16_t x, uint16_t y, uint8_t ch,
                      uint16_t fc, uint16_t bc, uint8_t sizey);
```
- **参数**:
  - `x, y`: 显示位置
  - `ch`: 字符（ASCII）
  - `fc`: 前景色（字符颜色）
  - `bc`: 背景色
  - `sizey`: 字体大小（12/16/24/32）
- **示例**:
```c
LCD_ShowChar_Hor(10, 10, 'A', WHITE, BLACK, 16);  // 显示白色字符'A'
```

#### `LCD_ShowString()` / `LCD_ShowString_Hor()` - 显示字符串
```c
void LCD_ShowString_Hor(uint16_t x, uint16_t y, const char *s,
                        uint16_t fc, uint16_t bc, uint8_t sizey);
```
- **示例**:
```c
LCD_ShowString_Hor(10, 30, "Hello", WHITE, BLACK, 16);
```

#### `LCD_ShowNum()` / `LCD_ShowNum_Hor()` - 显示整数
```c
void LCD_ShowNum_Hor(uint16_t x, uint16_t y, uint32_t num, uint8_t len,
                     uint16_t fc, uint16_t bc, uint8_t sizey);
```
- **参数**:
  - `num`: 要显示的数字
  - `len`: 显示位数（不足补0）
- **示例**:
```c
LCD_ShowNum_Hor(10, 50, 123, 5, WHITE, BLACK, 16);  // 显示 "00123"
```

#### `LCD_ShowFloatNum()` - 显示浮点数
```c
void LCD_ShowFloatNum(uint16_t x, uint16_t y, float num, uint8_t pre, uint8_t len,
                      uint16_t fc, uint16_t bc, uint8_t sizey);
```
- **参数**:
  - `pre`: 小数点后位数
  - `len`: 总位数
- **示例**:
```c
LCD_ShowFloatNum(10, 70, 3.14159, 2, 5, WHITE, BLACK, 16);  // 显示 "03.14"
```

---

### 3.5 中文显示函数

```c
void LCD_ShowChinese12x12(uint16_t x, uint16_t y, const char *s,
                          uint16_t fc, uint16_t bc, uint8_t sizey);
void LCD_ShowChinese16x16(...);
void LCD_ShowChinese24x24(...);
void LCD_ShowChinese32x32(...);
```
- **注意**: 需要在字库文件中预先定义中文字模数据

---

### 3.6 图片显示函数

#### `LCD_ShowPicture()` - 显示图片（普通方式）
```c
void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                     const uint8_t pic[]);
```

#### `LCD_ShowPicture_dma()` - 显示图片（DMA高速）⭐ 推荐
```c
void LCD_ShowPicture_dma(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                         const uint8_t pic[], const uint16_t data_size);
```
- **参数**:
  - `pic[]`: 图片数据数组（RGB565格式）
  - `data_size`: 数据大小（字节）
- **示例**:
```c
// 显示50x50的图片
LCD_ShowPicture_dma(10, 10, 50, 50, my_image_data, 50*50*2);
```

---

### 3.7 高级显示函数（带DMA加速）

这些函数使用DMA传输，速度更快，适合频繁刷新的场景：

```c
void LCD_ShowChar_Hor_dma(...)       // DMA显示字符
void LCD_ShowNum_Hor_dma(...)        // DMA显示数字
void LCD_ShowStr_Hor_dma(...)        // DMA显示字符串
void LCD_ShowPicture_batch(...)      // 批量显示图片
```

---

## 四、通信链路架构

```
┌─────────────────────────────────────────────────────────────┐
│                     应用层 (Application)                      │
│  rc_scheduler.c / main.c                                     │
│  - 调用LCD显示函数更新UI                                       │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  上层API (lcd_kmg.c)                         │
│  LCD_ShowString_Hor()                                        │
│  LCD_ShowNum_Hor_dma()                                       │
│  LCD_ShowPicture_dma()                                       │
│  - 处理字库、坐标计算、颜色转换                                 │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                 驱动层 (lcd_init.c)                          │
│  LCD_WR_REG()      - 写寄存器命令                             │
│  LCD_WR_DATA8()    - 写8位数据                               │
│  LCD_WR_DATA_dma() - DMA批量写数据                           │
│  LCD_Address_Set() - 设置显示窗口                             │
│  - 封装SPI通信协议                                            │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              SPI硬件驱动 (spi.c)                             │
│  spi_write()       - 阻塞式SPI发送                            │
│  spi_dma_write()   - DMA非阻塞发送                           │
│  - 控制SPI外设寄存器                                          │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              GPIO控制 (drv_gpio.c)                           │
│  gpio_set(LCD_CS)  - 片选控制                                │
│  gpio_set(LCD_DCX) - 数据/命令选择                            │
│  gpio_set(LCD_RST) - 复位控制                                │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  硬件引脚 (pinmap_v1.h)                       │
│  P04 - LCD_SPI_SCK  (SPI时钟)                                │
│  P05 - LCD_SPI_MOSI (SPI数据)                                │
│  P06 - LCD_SPI_CS   (片选)                                   │
│  P07 - LCD_DCX      (数据/命令)                               │
│  P20 - LCD_RST      (复位)                                   │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
              ┌──────────────┐
              │  SH8501A0    │
              │  驱动IC       │
              └──────┬───────┘
                     │
                     ▼
              ┌──────────────┐
              │  AMOLED屏幕  │
              │  128x240     │
              └──────────────┘
```

---

## 五、典型使用流程

### 5.1 初始化流程

```c
#include "lcd_init.h"
#include "lcd_kmg.h"

void lcd_display_init(void)
{
    // 1. 初始化SPI硬件（在spi.c中完成）
    spi_init();

    // 2. 初始化LCD（配置引脚、复位、发送初始化命令）
    OLED_Init();  // ⭐ 使用OLED_Init，因为你的屏幕是AMOLED

    // 3. 清屏
    LCD_Fill(0, 0, 127, 239, BLACK);

    // 4. 显示欢迎信息
    LCD_ShowString_Hor(10, 10, "Ready!", WHITE, BLACK, 16);
}
```

### 5.2 显示文字和数字

```c
void update_display(uint16_t speed, uint8_t battery)
{
    // 显示速度
    LCD_ShowString_Hor(10, 50, "Speed:", WHITE, BLACK, 16);
    LCD_ShowNum_Hor_dma(70, 50, speed, 3, WHITE, BLACK, 16);

    // 显示电量
    LCD_ShowString_Hor(10, 80, "Battery:", WHITE, BLACK, 16);
    LCD_ShowNum_Hor_dma(90, 80, battery, 3, WHITE, BLACK, 16);
    LCD_ShowString_Hor(120, 80, "%", WHITE, BLACK, 16);
}
```

### 5.3 显示图片

```c
// 图片数据（RGB565格式，50x50像素）
const uint8_t logo_50x50[5000] = { /* 图片数据 */ };

void show_logo(void)
{
    LCD_ShowPicture_dma(39, 95, 50, 50, logo_50x50, sizeof(logo_50x50));
}
```

---

## 六、颜色定义（RGB565格式）

```c
#define WHITE       0xFFFF  // 白色
#define BLACK       0x0000  // 黑色
#define RED         0xF800  // 红色
#define GREEN       0x07E0  // 绿色
#define BLUE        0x001F  // 蓝色
#define YELLOW      0xFFE0  // 黄色
#define CYAN        0x7FFF  // 青色
#define MAGENTA     0xF81F  // 洋红
#define GRAY        0x8430  // 灰色
```

---

## 七、性能优化建议

### 7.1 使用DMA函数提升速度

对于频繁刷新的内容（如数字、图片），优先使用带`_dma`后缀的函数：

```c
// 慢速（阻塞式）
LCD_ShowNum_Hor(10, 50, speed, 3, WHITE, BLACK, 16);

// 快速（DMA非阻塞）⭐ 推荐
LCD_ShowNum_Hor_dma(10, 50, speed, 3, WHITE, BLACK, 16);
```

### 7.2 减少不必要的刷新

只刷新变化的部分，而不是整屏刷新：

```c
// 不好：每次都清屏
LCD_Fill(0, 0, 127, 239, BLACK);
LCD_ShowNum_Hor_dma(10, 50, speed, 3, WHITE, BLACK, 16);

// 好：只更新数字区域
LCD_ShowNum_Hor_dma(10, 50, speed, 3, WHITE, BLACK, 16);
```

### 7.3 批量操作

使用`LCD_TX_Begin/End`包裹多个操作，减少CS切换次数：

```c
LCD_TX_Begin(LCD_TX_DATA);
// 多个写操作
LCD_TX_End();
```

---

## 八、常见问题

### Q1: 应该用LCD_Init还是OLED_Init？
**A**: 你的屏幕是AMOLED（ZJY095-2424AM002），应该使用`OLED_Init()`。

### Q2: 屏幕显示花屏或不正常？
**A**: 检查以下几点：
1. SPI引脚配置是否正确（参考pinmap_v1.h）
2. CS时序是否正确（参考LCD_SPI_CS_TIMING_FIX.md）
3. 初始化命令是否匹配驱动IC（SH8501A0）

### Q3: 显示速度慢？
**A**: 使用DMA函数（`_dma`后缀）并启用DMA配置：
```c
// oled_config.h
#define USE_DMA 1
```

### Q4: 如何显示自定义图片？
**A**:
1. 将图片转换为RGB565格式的C数组
2. 使用`LCD_ShowPicture_dma()`显示

---

## 九、参考资料

- **屏幕规格书**: `ZJY095-2424AM002-C15.pdf`
- **驱动IC手册**: `SH8501A0 DataSheet Preliminary V0.0(UCS)_210401 Truly.pdf`
- **SPI时序修复**: `LCD_SPI_CS_TIMING_FIX.md`
- **引脚定义**: `pinmap_v1.h`
