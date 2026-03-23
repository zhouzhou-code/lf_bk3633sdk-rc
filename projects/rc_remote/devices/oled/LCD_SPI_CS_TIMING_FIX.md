# LCD SPI 片选(CS)时序问题修复记录（BK3633 / 三线SPI无NSS）

## 背景与现象

工程里 LCD 走硬件 SPI（SCK/MOSI），CS 采用软件 GPIO 控制（因为三线模式 `WIRE3_EN=1` 时 **没有 NSS**）。

把 LCD 的片选脚（CS）改到新的 GPIO 后，出现：

- 屏幕显示异常/花屏/偶发正确
- 在 `src/device/lcd/lcd_init.c` 里强行加 `Delay_us(400)` 后“勉强正确”

这种“加固定延时就好”的典型信号，通常意味着：**CS 拉高太早，SPI 还没真正把数据移出去**。

## 根因分析（为什么会需要 400us）

### 1) LCD 层：CS 包裹了一个“非阻塞发送”

以 `LCD_WR_Bus_batch()`/`LCD_WR_DATA_Part()` 为例：

- `spi_dma_write()` 会启动 DMA + SPI 发送，然后立即返回
- 如果紧接着 `LCD_CS_Set()`，就会在 DMA/SPI 还在发送时释放 CS
- LCD 控制器会把后续时钟/数据当成“未选中”，导致截断、花屏

因此 CS 必须保持低电平直到 SPI 发送完成。

### 2) SPI 驱动层：`spi_send_state_get()` 的“done”信号被过早置位

工程里 LCD 使用 `spi_send_state_get()`/`spi_param.spi_state` 作为“发送完成”的标志。

问题在于 SPI 中断的来源不止“TX 完成”，还可能包含 FIFO 水位中断等：

- `TXFIFO_INT_EN` 是 **TX FIFO 阈值中断**（“FIFO 到阈值”），不是“发送完成”
- 如果 `spi_isr()` 把“任意 SPI 中断”都当成完成并把 `spi_state=1`，那么：
  - `lcd_spi_wait_done()` 会过早退出
  - CS 会提前拉高
  - 你看到的现象就会发生

额外加的 `Delay_us(400)`，本质是在“错误的 done 信号”之后再硬等一段时间，覆盖剩余传输时间，所以才会“勉强正确”。

## 修改点（按文件拆分）

### A) `src/device/lcd/lcd_init.c`

#### A1. 新增等待函数：确保 CS 覆盖完整传输

新增 `lcd_spi_wait_done(timeout_us)`：

- 轮询 `spi_send_state_get()`
- 在超时前一直等待 `spi_state` 从 0 -> 1（由 `spi_isr()` 在真正完成时置位）

目的：把“CS 时序正确性”从“经验延时”变成“明确等待完成事件”。

#### A2. 修复 CS 包裹逻辑（把 CS 释放时机后移）

涉及函数：

- `LCD_WR_Bus()`：`CS_Clr -> spi_write -> wait_done -> CS_Set`
- `LCD_WR_Bus_batch()`：`CS_Clr -> spi_dma_write -> wait_done -> (可选hold) -> CS_Set`
- `LCD_WR_DATA_Part()`：`CS_Clr -> spi_dma_write -> wait_done -> CS_Set`

核心变化：**CS 必须在 `wait_done` 之后才拉高**。

#### A3. 修复“DMA + 栈变量”的隐患（容易导致数据错乱）

`LCD_WR_Bus_16()` 之前使用类似：

- `uint8_t sd_buffer[2];`
- `spi_dma_write(&sd_buffer, 2, ...)`

这是不安全的：DMA 可能在函数返回后仍在读取该 buffer，而栈空间可能已被覆盖。

修复策略：

- 2 字节小数据改为 `spi_write(sd_buffer, 2)`（阻塞/短发送），避免 DMA 栈变量风险

#### A4. 确保控制脚方向正确（避免“gpio_set 只是改电平，不改方向”）

在 `LCD_Init()` 和 `OLED_Init()` 开头加入：

- `gpio_config(LCD_CS_GPIO_PIN, OUTPUT, PULL_HIGH)`（CS 默认拉高）
- `gpio_config(LCD_DC_GPIO_PIN, OUTPUT, ...)`
- `gpio_config(LCD_RES_GPIO_PIN, OUTPUT, ...)`
- `gpio_config(LCD_KEY1_GPIO_PIN, OUTPUT, ...)`

目的：当你更换了 CS 引脚时，不会因为没有配置成输出导致 CS 实际没被正确拉动。

#### A5. 说明：仍保留的短延时

当前代码里仍有少量 `Delay_us(...)`（例如 batch 写后有一个 hold）：

- 它不是“代替 done 等待”，而是“最后一拍后的 hold time/器件容忍度”兜底
- 如果你用逻分确认 CS/SCK 完全满足时序，可以再把这些延时缩短到更合理值（通常是 0~几微秒量级）

### B) `src/driver/spi/spi.c`

#### B1. 禁用 `TXFIFO_INT_EN`（避免中途触发 ISR 影响 done 判断）

在 `spi_init()` 中将：

- `POS_SPI_REG0X0_TXFIFO_INT_EN` 置 0

原因：TXFIFO 阈值中断不是“发送完成”，会在传输中途触发；如果用它驱动“完成”逻辑，会直接破坏软件 CS 时序。

#### B2. 修复 `spi_isr()`：只在真正完成时才置 `spi_state=1`

新的处理逻辑：

- 先读取 `SPI_REG0X2_STAT`
- 清掉 FIFO level / 溢出等状态位（不作为完成）
- 只有当检测到：
  - `TX_FINISH_INT` 或
  - `RX_FINISH_INT`
  才会：
  - 清对应完成标志
  - `spi_param.spi_state = 1`
  -（按原设计）可调用 `write_complete_cb`
  - 关闭 SPI 中断使能位（`SYS_REG0X10_INT_EN` 的 SPI bit）

这样 `spi_send_state_get()` 才能真实表达“本次配置长度的传输已完成”。

## 与芯片文档/表格的对应关系

来自 `BK3633_ADDR Mapping.xlsm` 的 `SPI0` 页：

- `WIRE3_EN`：`1：三线模式（没有 NSS）`

因此当 `WIRE3_EN=1` 时：

- 必须使用 GPIO 手动控制 CS（或让器件一直选中）
- CS 的拉低/拉高时机就变成正确显示的关键

## 验证建议（强烈建议用逻辑分析仪）

1) 抓 `CS/SCK/MOSI`
2) 验证以下条件：
   - CS 在一整段 SCK 期间保持低电平
   - CS 只在最后一个 SCK 后沿（或控制器要求的边沿）之后拉高
3) 做压力测试：
   - 连续刷屏（`LCD_Fill`/图片绘制/批量数据）
   - 观察是否还有随机花屏/丢行

如果还有问题，下一步通常是检查：

- SPI 时钟频率（`SPI_CKR`）是否超过 LCD 可接受范围
- `CKPHA/CKPOL` 是否与 LCD 控制器模式匹配
- 你新换的 CS GPIO 是否被其它模块复用/被中断或其他代码翻转

## 可能的后续优化（可选）

- 把 “CS 拉高” 放到 DMA 完成回调里（`spi_dma_write(..., cb)`），避免轮询等待占用 CPU
- 为 LCD 封装一个 “begin/end transaction” API，把 CS/DC/发送组合成事务，减少散落的 CS 控制

## 已落地的进一步优化（2026-01）

本次已将上面两条“可选优化”落地到代码中，核心目标是：

- DMA 发送时，CS 的释放点由“函数返回后硬延时/轮询”改为“DMA/SPI 真正完成时的 ISR 回调”
- 业务代码不再直接操作 `CS/DC + spi_dma_write()`，统一走 LCD 的 Transaction API

### 1) CS 拉高放到 DMA 完成回调（避免占用 CPU）

实现位置：`src/device/lcd/lcd_init.c`

- 新增 LCD 发送状态机：`s_lcd_tx_busy / s_lcd_tx_done / s_lcd_tx_end_pending / s_lcd_tx_depth`
- DMA 发送时：
  - `LCD_TX_End()` 如果发现仍在发送（busy=1），只打标记 `end_pending=1`
  - 真正 `TX_FINISH_INT` 到来时（SPI ISR -> 回调），才执行 `LCD_CS_Set()` 释放片选
- 等待完成时：使用 `cpu_idle_sleep()` 进入低功耗等待中断唤醒（比 `Delay_us(1)` 轮询更省 CPU）

备注：实际在本工程/当前芯片低功耗配置下，进入 `cpu_idle_sleep()` / `WFI` 会导致 SPI/DMA 时钟被门控（表现为刷新极慢、功耗掉到 ~10uA、且花屏）。
因此 `LCD_TX_WaitDone()` 最终采用 `Delay_us(1)` 的短轮询，确保传输期间时钟持续运行。

### 2) LCD “begin/end transaction” API（集中管理 CS/DC/发送）

对外接口（声明在 `src/device/lcd/lcd_init.h`）：

```c
typedef enum { LCD_TX_CMD = 0, LCD_TX_DATA = 1 } lcd_tx_dc_t;
void LCD_TX_Begin(lcd_tx_dc_t dc);
void LCD_TX_Write_DMA_Async(const uint8_t *buf, uint16_t len, lcd_tx_done_cb_t done_cb);
void LCD_TX_End(void);
void LCD_TX_WaitDone(void);
```

使用方式（推荐模式：End 在 Wait 之前调用，这样 CS 的拉高发生在 ISR 回调里）：

```c
LCD_TX_Begin(LCD_TX_DATA);
LCD_TX_Write_DMA_Async(buf, len, NULL);
LCD_TX_End();       // 先 End：若 DMA 未完成，则 CS 由回调释放
LCD_TX_WaitDone();  // 再 Wait：CPU idle sleep 等待中断唤醒
```

业务侧落地：`src/device/lcd/lcd_kmg.c` 中原先直接 `spi_dma_write()` 的位置改为调用上述 API，减少散落的 CS/DC 控制。

## DC（D/C 引脚）是做什么的？

LCD 的 D/C（Data/Command）引脚用于区分 SPI 总线上传输的字节含义：

- `D/C = 0`（本工程里 `LCD_DC_Clr()`）：本次传输的字节被 LCD 控制器当成“命令/寄存器索引”（例如 `0x2A/0x2B/0x2C` 等）
- `D/C = 1`（本工程里 `LCD_DC_Set()`）：本次传输的字节被当成“数据”（命令的参数、像素数据/GRAM 数据等）

因此，典型写屏流程是：

1) `D/C=0` 发送命令（例如设置列地址、行地址、开始写入 GRAM）
2) `D/C=1` 发送参数/像素流数据
