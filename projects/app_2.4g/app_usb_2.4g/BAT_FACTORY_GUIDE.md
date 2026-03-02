# 电池厂工程师测试操作说明（2.4G 透传/配对）

本文档说明同一份固件在 **Host/Slave** 两种模式下的测试步骤与注意事项。

## 1. 硬件与接口

- **SET 测试模式脚**：`P0.2`  
  - 低电平：`Slave`(电池透传固件，引脚默认下拉进入该模式)
  - 高电平：`Host` (遥控器固件，透传接收方)
- **PD 配对按键**：`P0.3`（低电平有效）
- **串口**：`UART0` 输出业务调试日志与配对结果反馈帧  
  - 参数：`115200, 8N1`

## 2. Host/Slave 运行模式

同一份固件可通过 `SET(P0.2)` 切换：

- **Host 模式（SET=高）**
  - 按下 **PD 按键** 才开始配对
  - 配对完成后进入 RX，打印无线rf模块接收到的电池 SOC
- **Slave 模式（SET=低）**
  - 透传为主流程
  - 配对触发条件：**PD 按下 + 串口收到配对命令帧**

## 3. 配对命令与应答

**Slave 侧配对命令帧（Host 发送给 Slave）**

```
Header: 0xAA 0x55
Cmd:    0xAA  (PAIR_CMD)
Addr:   0xXX  (设备地址)
Len:    0x01
Data:   0x00
CRC16:  Modbus CRC (低字节在前)
```

**Slave 配对结果应答（Slave 通过串口发送）**

```
Header: 0xAA 0x55
Cmd:    0xBB  (PAIR_RESP)
Addr:   0xXX
Len:    0x01
Data:   0x00  成功
        0x01  失败
CRC16:  Modbus CRC (低字节在前)
```

## 4. 推荐测试流程

### 4.1 Host/Slave 配对验证

1. Host 板：`SET(P0.2)=高`，上电  
2. Slave 板：`SET(P0.2)=低`，上电  
3. Host 板按下 **PD** 进入配对  
4. Slave 板按下 **PD**，并通过串口发送 `PAIR_CMD`  
5. Slave 侧串口应答 `PAIR_RESP`（00 成功 / 01 失败）

### 4.2 透传与接收打印验证

- Slave 收到电池动态帧后会 RF 转发 SOC
- Host 接收到 SOC 后串口打印：  
  - `rf soc=xx%`

## 5. 关键日志（UART0）

以下日志均为 `UART0` 输出：

- `Mode=HOST (SET=P0.2 high)`  
- `Mode=SLAVE (SET=P0.2 low)`  
- `PAIR_IO low`  
- `PAIR_CMD received`  
- `Enter pairing mode`  
- `Pair result=SUCCESS/FAIL`  
- `rf soc=xx%`

## 6. 常见问题排查

- **无配对应答**  
  - 检查 `SET` 模式脚电平  
  - Host 是否按下 PD  
  - Slave 是否按下 PD  
  - `PAIR_CMD` 帧的 CRC 是否正确  

- **Host 没有收到 SOC**  
  - 确认配对成功  
  - 确认 Host 进入 RX（会打印 `Host pairing done`）

如需更多调试信息，请告知需要增加的日志内容或条件。
