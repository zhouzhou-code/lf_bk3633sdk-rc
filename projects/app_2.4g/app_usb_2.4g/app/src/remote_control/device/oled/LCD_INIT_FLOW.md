# LCD屏幕驱动完整初始化流程

## 一、初始化顺序（重要！）⭐

要驱动LCD屏幕，必须按照以下顺序初始化：

```
1. GPIO初始化
   ↓
2. SPI硬件初始化
   ↓
3. DMA初始化（自动，包含在SPI驱动中）
   ↓
4. LCD驱动IC初始化
   ↓
5. 开始使用LCD显示函数
```

---

## 二、详细初始化步骤

### 步骤1：GPIO初始化（系统自动完成）

GPIO在系统启动时已经初始化，不需要手动调用。

**位置**：`main.c` 中的 `gpio_init()`

```c
// 系统启动时自动调用
gpio_init();  // 初始化GPIO系统
```

---

### 步骤2：SPI硬件初始化 ⭐ **必须手动调用**

**函数**：`spi_init(mode, freq_div, bit_wdth)`

**位置**：`src/driver/spi/spi.c`

**参数说明**：
- `mode`: SPI模式
  - `0` = 主机模式（Master）⭐ LCD使用这个
  - `1` = 从机模式（Slave）

- `freq_div`: 时钟分频系数
  - SPI时钟 = PLL时钟 / (freq_div × 2)
  - `0` = 不分频（最快，PLL/2）
  - `1` = 2分频（PLL/4）
  - `2` = 4分频（PLL/8）
  - 推荐值：`0`（最快）或 `2`（稳定）

- `bit_wdth`: 数据位宽
  - `0` = 8位 ⭐ LCD使用这个
  - `1` = 16位

**示例**：
```c
// 在main.c中调用
#if(SPI_DRIVER)
    spi_init(0, 0, 0);  // 主机模式，不分频，8位
    uart_printf("init spi\\r\\n");
#endif
```

**spi_init()做了什么**：
1. 使能SPI时钟
2. 配置GPIO引脚为SPI第二功能（P04-P07）
3. 配置SPI寄存器（时钟极性、相位、位宽等）
4. 选择PLL时钟源
5. 初始化SPI状态变量

---

### 步骤3：DMA初始化（自动完成）

DMA不需要单独初始化，在使用时自动配置。

**DMA配置函数**：`dma_config_write()` 和 `dma_config_read()`

**位置**：`src/driver/dma/dma.c`

**何时调用**：
- 当调用 `spi_dma_write()` 时，会自动调用 `dma_config_write()`
- DMA配置包括：源地址、目标地址、传输长度、数据宽度等

**DMA通道分配**：
- LCD使用DMA通道0
- 请求源：`SPI_REQ`
- 传输方向：内存 → SPI数据寄存器

---

### 步骤4：LCD驱动IC初始化 ⭐ **必须手动调用**

**函数**：`OLED_Init()` 或 `LCD_Init()`

**位置**：`app/src/remote_control/device/oled/lcd_init.c`

**选择哪个函数**：
- **OLED_Init()** ⭐ 推荐，适用于AMOLED/OLED屏幕（你的屏幕）
- **LCD_Init()** - 适用于TFT LCD屏幕

**示例**：
```c
#if(ENABLE_LED_DISPLAY)
    OLED_Init();  // 初始化OLED屏幕
    uart_printf("init oled\\r\\n");
#endif
```

**OLED_Init()做了什么**：
1. 配置控制引脚（CS、DCX、RST）为GPIO输出
2. 执行硬件复位（RST引脚时序）
3. 发送驱动IC初始化序列（几百行寄存器配置）
4. 使能显示输出

---

### 步骤5：清屏（可选但推荐）

初始化后建议清屏，避免显示随机数据。

```c
LCD_Fill(0, 0, 127, 239, BLACK);  // 全屏填充黑色
```

---

## 三、完整初始化代码示例

### 在main.c中的完整流程

```c
void user_init(void)
{
    // ... 其他初始化代码 ...

    // 1. GPIO初始化（系统自动完成）
    gpio_init();

    // 2. SPI硬件初始化
    #if(SPI_DRIVER)
    spi_init(0, 0, 0);  // 主机模式，不分频，8位
    uart_printf("init spi\\r\\n");
    #endif

    // 3. DMA初始化（自动完成，不需要手动调用）

    // 4. LCD驱动IC初始化
    #if(ENABLE_LED_DISPLAY)
    OLED_Init();  // 初始化OLED屏幕
    uart_printf("init oled\\r\\n");

    // 5. 清屏
    LCD_Fill(0, 0, LCD_W-1, LCD_H-1, BLACK);

    // 6. 显示测试内容
    LCD_ShowString_Hor(10, 10, "Hello!", WHITE, BLACK, 16);
    LCD_ShowNum_Hor_dma(10, 30, 123, 3, WHITE, BLACK, 16);
    #endif

    // ... 其他初始化代码 ...
}
```

---

## 四、初始化相关的宏定义

### user_config.h

```c
#define SPI_DRIVER              1  // 使能SPI驱动（硬件SPI）
#define ENABLE_LED_DISPLAY      1  // 使能LCD显示功能
```

### oled_config.h

```c
#define USE_HORIZONTAL 0  // 显示方向：0=竖屏，1=横屏
#define LCD_W 120         // 屏幕宽度（像素）
#define LCD_H 240         // 屏幕高度（像素）
#define USE_DMA 1         // 使能DMA传输
```

---

## 五、初始化时序图

```
系统启动
    ↓
main()
    ↓
user_init()
    ↓
┌─────────────────────────────────────┐
│ 1. gpio_init()                      │ ← 系统自动
│    - 初始化GPIO系统                  │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 2. spi_init(0, 0, 0)                │ ← 手动调用
│    - 使能SPI时钟                     │
│    - 配置GPIO为SPI功能               │
│    - 配置SPI寄存器                   │
│    - 选择PLL时钟                     │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 3. OLED_Init()                      │ ← 手动调用
│    - 配置CS/DCX/RST引脚              │
│    - 硬件复位（RST时序）             │
│    - 发送初始化序列                  │
│    - 使能显示                        │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 4. LCD_Fill(0,0,127,239,BLACK)      │ ← 可选
│    - 清屏                            │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ 5. 使用LCD显示函数                   │
│    - LCD_ShowString_Hor()           │
│    - LCD_ShowNum_Hor_dma()          │
│    - LCD_ShowPicture_dma()          │
└─────────────────────────────────────┘
```

---

## 六、DMA工作原理

### DMA在LCD驱动中的作用

当你调用带DMA的函数时（如`LCD_ShowPicture_dma()`），内部流程如下：

```
1. 应用层调用
   LCD_ShowPicture_dma(x, y, w, h, pic_data, size)
   ↓
2. LCD驱动层
   LCD_WR_DATA_dma(pic_data, size)
   ↓
3. SPI驱动层
   spi_dma_write(pic_data, size, callback)
   ↓
4. DMA驱动层
   dma_config_write(0, SPI_REQ, pic_data, SPI_REG, size, DMA_DW_8B)
   ↓
5. DMA硬件
   - 配置DMA通道0
   - 源地址：pic_data（内存）
   - 目标地址：SPI_REG0X3_DAT（SPI数据寄存器）
   - 传输长度：size字节
   - 启动DMA传输
   ↓
6. DMA自动搬运数据
   - CPU不参与，继续执行其他任务
   - DMA硬件自动将数据从内存搬到SPI寄存器
   - SPI硬件自动发送数据到LCD
   ↓
7. 传输完成中断
   - DMA完成后触发中断
   - 调用回调函数 lcd_spi_tx_done_isr_cb()
   - 释放CS片选
   - 通知应用层传输完成
```

### DMA配置参数

```c
dma_config_write(
    0,              // DMA通道0
    SPI_REQ,        // 请求源：SPI
    (uint32_t)buf,  // 源地址：数据缓冲区
    SPI_REG0X3_DAT, // 目标地址：SPI数据寄存器
    size,           // 传输长度（字节）
    DMA_DW_8B       // 目标数据宽度：8位
);
```

---

## 七、常见问题

### Q1: 为什么必须先初始化SPI再初始化LCD？

**A**: 因为LCD初始化函数（OLED_Init）内部会调用SPI通信函数（LCD_WR_REG、LCD_WR_DATA8）来配置驱动IC。如果SPI没有初始化，这些函数会失败。

### Q2: DMA需要单独初始化吗？

**A**: 不需要。DMA在第一次使用时（调用spi_dma_write）会自动配置。每次DMA传输都会重新配置通道参数。

### Q3: 如果不使用DMA，LCD能工作吗？

**A**: 可以，但速度会慢很多。可以通过设置 `USE_DMA = 0` 来禁用DMA，此时会使用FIFO模式传输。

### Q4: spi_init的参数应该怎么设置？

**A**: 推荐设置：
```c
spi_init(0, 0, 0);  // 主机模式，不分频（最快），8位
// 或
spi_init(0, 2, 0);  // 主机模式，4分频（更稳定），8位
```

### Q5: 初始化失败怎么办？

**A**: 检查以下几点：
1. 确认 `SPI_DRIVER = 1` 和 `ENABLE_LED_DISPLAY = 1`
2. 确认引脚连接正确（参考pinmap_v1.h）
3. 确认初始化顺序正确（先SPI后LCD）
4. 检查串口输出是否有错误信息
5. 使用示波器检查SPI信号是否正常

---

## 八、性能优化建议

### 1. 使用DMA函数

优先使用带`_dma`后缀的函数：
```c
// 慢速（阻塞）
LCD_ShowNum_Hor(10, 50, speed, 3, WHITE, BLACK, 16);

// 快速（DMA）⭐ 推荐
LCD_ShowNum_Hor_dma(10, 50, speed, 3, WHITE, BLACK, 16);
```

### 2. 调整SPI时钟

如果显示正常，可以尝试提高SPI时钟：
```c
spi_init(0, 0, 0);  // 不分频，最快（PLL/2）
```

如果显示异常，降低SPI时钟：
```c
spi_init(0, 2, 0);  // 4分频，更稳定（PLL/8）
```

### 3. 批量操作

尽量使用批量函数，减少CS切换次数：
```c
// 不好：多次单独操作
LCD_ShowChar_Hor(10, 10, 'H', WHITE, BLACK, 16);
LCD_ShowChar_Hor(20, 10, 'i', WHITE, BLACK, 16);

// 好：使用字符串函数
LCD_ShowString_Hor(10, 10, "Hi", WHITE, BLACK, 16);
```

---

## 九、总结

### ✅ 必须手动调用的初始化函数

1. **spi_init(0, 0, 0)** - 初始化SPI硬件
2. **OLED_Init()** - 初始化LCD驱动IC

### ❌ 不需要手动调用的

1. **gpio_init()** - 系统自动调用
2. **dma_config_write()** - SPI驱动自动调用

### 🎯 最简初始化代码

```c
// 在main.c的user_init()中添加：
spi_init(0, 0, 0);                      // 初始化SPI
OLED_Init();                            // 初始化LCD
LCD_Fill(0, 0, 127, 239, BLACK);        // 清屏
LCD_ShowString_Hor(10, 10, "OK!", WHITE, BLACK, 16);  // 测试显示
```

就这么简单！
