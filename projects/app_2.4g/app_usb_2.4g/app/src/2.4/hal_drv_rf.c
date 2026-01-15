#include <stdio.h>
#include "driver_bk24.h"
#include "driver_timer.h"
#include "bk3633_reglist.h"

#define UART_PRINTF    uart_printf
int uart_printf(const char *fmt,...);

SYSTEM_STRUCT_DATA system_data;
uint32_t RF_flag=0;



// 配置模拟寄存器，固定值不可改
static uint32_t Cfg_0c_Val[4] = {0x1F, 0x1B, 0x73, 0x00};
static uint32_t Cfg_0d_Val[4] = {0x36, 0xB4, 0x80, 0x28};


#include "hal_drv_rf.h"
#include "bk3633_reglist.h" // 包含硬件寄存器定义
// #include "driver_timer.h" // 如果需要微秒延时函数

static void RF_WriteReg(uint8_t cmd, uint8_t value) 
{
}

static void RF_WriteBuf(uint8_t cmd, uint8_t *pBuf, uint8_t len) 
{
    uint8_t i;
    TRX_CMD = cmd;
    for(i = 0; i < len; i++) {
        TRX_FIFO = pBuf[i];
    }
    TRX_CMD = 0x00; // NOP / End translation
}

static void RF_ReadBuf(uint8_t cmd, uint8_t *pBuf, uint8_t len) 
{
    uint8_t i;
    TRX_CMD = cmd;
    for(i = 0; i < len; i++) {
        pBuf[i] = TRX_FIFO;
    }
    TRX_CMD = 0x00;
}

static void RF_MemCpy(volatile uint32 *pDest, uint32_t *pSrc, uint8_t len) 
{
    uint8_t i;
    for (i = 0; i < len; i++) 
    {
        pDest[i] = pSrc[i];
    }
}

static void RF_DelayUs(volatile uint32_t us) 
{
    volatile uint32_t i;
    for(i=0; i<us*10; i++); 
}


/*---------------------HAL 接口实现------------------*/

/**
  * @brief  初始化 RF 模块
  * @param  hrf: RF 句柄
  */
HAL_StatusTypeDef HAL_RF_Init(RF_HandleTypeDef* hrf,RF_ConfgTypeDef *Init)
{
    if (Init == NULL || hrf == NULL) return HAL_ERROR;

    hrf->State = HAL_RF_STATE_BUSY;
    
    // 1. 底层硬件复位/时钟开启 (MSP Params)
    //HAL_RF_MspParams(hrf);
    
    // 2. 清除外设电源掉电位 (Enable Peripheral Clock)
    clrf_SYS_Reg0x3_bk24_pwd;

    /* 配置默认模拟寄存器参数 (Magic Numbers) */
    RF_MemCpy(&TRX_CFG_0C_0, (uint32_t*)Cfg_0c_Val, 4);
    RF_MemCpy(&TRX_CFG_0D_0, (uint32_t*)Cfg_0d_Val, 4);


    TRX_CONFIG = 0x0C; // Default: PTX mode, CRC(2 bytes) enabled, PWR_UP=0
    
    /* tx / rx配置 */
    if (Init->Mode == MODE_TX) {
        TRX_CONFIG &= ~(1<<0); // Clear PRIM_RX bit for TX mode
    } else if (Init->Mode == MODE_RX) {
        TRX_CONFIG |= (1<<0);  // Set PRIM_RX bit for RX mode
    } else {
        return HAL_ERROR;
    }

    
    TRX_CONFIG |= 0x02; // Set PWR_UP bit to power up the device
    // 设置地址宽度 


    /* ---------------- 地址 ---------------- */
    uint8_t tmp_addr_width=Init->Address.AddressWidth;

    //地址宽度(SETUP_AW: 01=3bytes, 10=4bytes, 11=5bytes)
    if(tmp_addr_width < 3 || tmp_addr_width > 5)
        return HAL_ERROR; // 地址宽度无效

    TRX_SETUP_AW = (tmp_addr_width - 2) & 0x03; // 3~5 bytes
    
    // 设置 TX 发送地址
    RF_MemCpy(&TRX_TX_ADDR_0, (uint32_t*)Init->Address.TxAddress, tmp_addr_width);
    // RX Pipe 0 (通常设为等于 TX 地址用于接收 ACK)
    RF_MemCpy(&TRX_RX_ADDR_P0_0, (uint32_t*)Init->Address.RxAddrP0, tmp_addr_width);
    // RX Pipe 1 
    RF_MemCpy(&TRX_RX_ADDR_P1_0, (uint32_t*)Init->Address.RxAddrP1, tmp_addr_width);
    // RX Pipe 2-5
    TRX_RX_ADDR_P2 = Init->Address.RxAddrP2;
    TRX_RX_ADDR_P3 = Init->Address.RxAddrP3;
    TRX_RX_ADDR_P4 = Init->Address.RxAddrP4;
    TRX_RX_ADDR_P5 = Init->Address.RxAddrP5;

    /* ---------------- 协议 ---------------- */
    
    // 自动重传设置 (SETUP_RETR)
    // 高4位: ARD (Delay, step 250us), 低4位: ARC (Count)
    TRX_SETUP_RETR = ((Init->Protocol.AutoRetransmitDelay & 0x0F) << 4) | 
                     (Init->Protocol.AutoRetransmitCount & 0x0F);

    // 接收通道与自动应答使能
    TRX_EN_RXADDR = Init->Protocol.RxPipesEn_Mask;        // 启用选定的 RX Pipes
    TRX_EN_AA     = Init->Protocol.RxPipesAutoAckEn_Mask; // 启用选定的 AutoAck

    // 动态/固定包长配置
    if (Init->Protocol.DynamicPayloadEnable)
    {
        // 启用动态包长
        TRX_FEATURE |= (1<<2); // Set EN_DPL (Enable Dynamic Payload Length)
        // 为所有开启了 AA 的通道启用动态包长 (DPL 依赖于 AA)
        TRX_DYNPD = Init->Protocol.RxPipesEn_Mask & 0x3F; 

        TRX_FEATURE |= 0x03; // EN_ACK_PAY | EN_DYN_ACK
    }
    else
    {
        // 禁用动态包长，使用固定包长
        TRX_FEATURE &= ~(1<<2); // Clear EN_DPL
        TRX_DYNPD = 0x00;
        uint8_t RxPipesEn_Mask = Init->Protocol.RxPipesEn_Mask;
        uint8_t PW = Init->Protocol.PayloadWidth;
        // 设置所有开启通道的固定载荷宽度
        if(RxPipesEn_Mask & (1<<0)) TRX_RX_PW_P0 = PW;
        if(RxPipesEn_Mask & (1<<1)) TRX_RX_PW_P1 = PW;
        if(RxPipesEn_Mask & (1<<2)) TRX_RX_PW_P2 = PW;
        if(RxPipesEn_Mask & (1<<3)) TRX_RX_PW_P3 = PW;
        if(RxPipesEn_Mask & (1<<4)) TRX_RX_PW_P4 = PW;
        if(RxPipesEn_Mask & (1<<5)) TRX_RX_PW_P5 = PW;
    }

    /* ---------------- 射频物理层配置 ---------------- */
    
    //频道
    //TRX_RF_CH =hrf->CurrentChannel; //还不太理解这个参数，姑且写着0x05
    TRX_RF_CH =0x05;

    __HAL_RF_PowerDown();

    /* XVR 模拟参数初始化 */
    addXVR_Reg0x24 = 0x000e0782;
    addXVR_Reg0x3b = 0x36340048;

    /* 数据速率 */
    switch ( Init->DataRate )
    {
        case 0 :  // 250kbps
            addXVR_Reg0x2e &= 0xfffffe00 ;
            addXVR_Reg0x2e |= 0x100 ;
            addXVR_Reg0x26 &= 0xe000ffff;
            addXVR_Reg0x26 |= 0x04080000;
            addXVR_Reg0x30 &= ~(0x1ff<<8);
            addXVR_Reg0x30 |= 0x28<<8;
            TRX_RF_SETUP = 0x27;
            break;
        case 1 : // 1Mbps
            addXVR_Reg0x2e &= 0xfffffe00 ;
            addXVR_Reg0x2e |= 0x100 ;
            addXVR_Reg0x26 &= 0xe000ffff;
            addXVR_Reg0x26 |= 0x10200000;
            TRX_RF_SETUP = 0x07;
            break;
        case 2 : // 2Mbps
            addXVR_Reg0x2e &= 0xfffffe00 ;
            addXVR_Reg0x2e |= 0x100 ;
            addXVR_Reg0x26 &= 0xe000ffff;
            addXVR_Reg0x26 |= 0x10200000;
            TRX_RF_SETUP = 0x0F;
            break;
        default:
            return HAL_ERROR;
            break;
    }

    xvr_reg_initial_24(); // 配置 XVR 寄存器

    __HAL_RF_PowerUp();
    __HAL_RF_CMD_FLUSH_RXFIFO();
    __HAL_RF_CMD_FLUSH_TXFIFO();
    __HAL_RF_CLEAR_IRQ_FLAGS((RX_DR|TX_DS|MAX_RT|RX_P_NO));

    // 3. 初始化 RF_HandleTypeDef 结构体
    hrf->Params = *Init;

    return HAL_OK;
    //hrf->State = HAL_RF_STATE_READY;
}
/** 
  * @brief 弱定义的 MSP Params (MCU特定配置，如时钟、GPIO)
  */
// __attribute__((weak)) void HAL_RF_MspParams(RF_HandleTypeDef *hrf)
// {
//     // Prevent unused warning
//     (void)hrf;
//     // 用户在应用层实现此函数，开启时钟等
// }

/* IO 操作函数 */

/**
  * @brief  阻塞式发送数据
  * @return 状态 HAL_RF_STATE_READY (成功) 或 HAL_RF_STATE_TIMEOUT/ERROR
  */
HAL_RF_StateTypeDef HAL_RF_Transmit(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size)
{

    // uint8_t status;
    // uint32_t timeoutCount = 0;
    // const uint32_t MAX_TIMEOUT = 1000; // 简单计数超时

    // if (hrf->State != HAL_RF_STATE_READY) return hrf->State;

    // hrf->State = HAL_RF_STATE_BUSY_TX;

    // // 1. 准备发送
    // HAL_RF_SetTxMode(hrf); 
    
    // // 2. 写入数据到 FIFO (Cmd 0x60: W_TX_PAYLOAD)
    // RF_WriteBuf(0x60, pData, Size);

    // // 3. 产生高脉冲启动发送 (如果 CE 已经在 SetTxMode 拉高，这里可能不需要，参考时序)
    // // 原代码逻辑：SwitchToTxMode 里 TRX_CE = 1 已经拉高

    // // 4. 等待发送完成或超时
    // do {
    //     status = TRX_IRQ_STATUS;
    //     RF_DelayUs(5);
    //     timeoutCount++;
    // } while ( ((status & (TX_DS | MAX_RT)) == 0) && (timeoutCount < MAX_TIMEOUT) );

    // // 5. 结果处理
    // if (status & TX_DS) {
    //     // 发送成功
    //     __HAL_RF_CLEAR_IRQ_FLAGS(TX_DS);
    //     hrf->State = HAL_RF_STATE_READY;
    // } 
    // else if (status & MAX_RT) {
    //     // 达到最大重传
    //     HAL_RF_ClearIRQ(hrf, MAX_RT);
    //     HAL_RF_FlushTX(hrf); // 清除发不出去的数据
    //     hrf->State = HAL_RF_STATE_TIMEOUT;
    //     hrf->ErrorCode = MAX_RT;
    // }
    // else {
    //     // 软件超时
    //     HAL_RF_FlushTX(hrf);
    //     hrf->State = HAL_RF_STATE_TIMEOUT;
    // }
    // return hrf->State;
}

/**  待调试
  * @brief  接收数据查询
  * @param  pData: 接收缓冲区
  * @param  Size:  期望读取长度 (如果 FIFO 数据小于此长度，将只读取实际长度)
  * @return HAL_RF_STATE_READY (有数据) 或 HAL_RF_STATE_BUSY_RX (无数据)
  */
HAL_RF_StateTypeDef HAL_RF_Receive(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size)
{
    // uint8_t status;
    // uint8_t rx_len;

    // status = TRX_IRQ_STATUS;

    // if (status & RX_DR) {
    //     // 1. 读取接收载荷宽度
    //     rx_len = TRX_RX_RPL_WIDTH;

    //     if (rx_len > RF_MAX_PACKET_LEN) {
    //         HAL_RF_FlushRX(hrf); // 异常数据，清空
    //         HAL_RF_ClearIRQ(hrf, RX_DR);
    //         return HAL_RF_STATE_ERROR;
    //     }

    //     // 2. 读取数据 (Cmd 0x40: R_RX_PAYLOAD)
    //     uint8_t readLen = (rx_len < Size) ? rx_len : Size;
    //     RF_ReadBuf(0x40, pData, readLen);

    //     hrf->LastRxLen = rx_len;

    //     // 3. 清除中断
    //     HAL_RF_ClearIRQ(hrf, RF_STATUS_RX_DR);

    //     // 4. 检查 FIFO 是否还有数据，如果为空则真正清除 RX_DR 逻辑通常由硬件自动处理
    //     // 但这里显式清除是个好习惯
        
    //     return HAL_RF_STATE_READY; // Data Received
    // }

    // return HAL_RF_STATE_BUSY_RX; // No Data
}


/* 控制函数实现 */

/* 不太理解这个，还需要斟酌 */
void HAL_RF_SetChannel(RF_HandleTypeDef *hrf, uint8_t channel)
{
    hrf->CurrentChannel = channel;
    __HAL_RF_CHIP_DIS();
    TRX_RF_CH = 0x80 + channel; 
    __HAL_RF_CHIP_EN();
}

/**
  * @brief  设置发送地址
  * @param  hrf: RF 句柄
  * @param  dev_addr: 发送地址指针
  * @detail 地址宽度由 hrf->Params.AddressWidth 决定
  */
void HAL_RF_SetTxAddress(RF_HandleTypeDef *hrf, uint32_t *dev_addr)
{
    // hrf->Params.Address.TxAddress = dev_addr;
    // RF_MemCpy(&TRX_TX_ADDR_0, dev_addr, hrf->Params.AddressWidth);
}

/**
  * @brief  设置发送地址
  * @param  hrf: RF 句柄
  * @param  dev_addr: 发送地址指针
  * @detail 地址宽度由 hrf->Params.AddressWidth 决定
  */
void HAL_RF_SetRxMode(RF_HandleTypeDef *hrf)
{
    __HAL_RF_PowerUp();
    __HAL_RF_CMD_FLUSH_RXFIFO();

    __HAL_RF_CHIP_DIS();
    __HAL_RF_Set_RxMode();
    __HAL_RF_CHIP_EN();

    hrf->State = HAL_RF_STATE_BUSY_RX;
}

// 设置为发送模式
void HAL_RF_SetTxMode(RF_HandleTypeDef *hrf)
{
    __HAL_RF_PowerUp();
    __HAL_RF_CMD_FLUSH_TXFIFO();

    __HAL_RF_CHIP_DIS();
    __HAL_RF_Set_TxMode();
    __HAL_RF_CHIP_EN();

    hrf->State = HAL_RF_STATE_BUSY_TX;
}
/**
  * @brief  清除指定的中断标志
  * @param  hrf: RF 句柄
  * @param  _Flags: 要清除的中断标志位掩码组合 IRQ_StatusBitsTypeDef
  */
void HAL_RF_ClearIRQFlags(RF_HandleTypeDef *hrf, IRQ_StatusBitsTypeDef _Flags)
{
    (void)hrf;
    // 写 1 清除
    TRX_IRQ_STATUS = _Flags;
}

/**
  * @brief  检查某个中断标志位是否置位
  * @param  hrf: RF 句柄
  * @param  Flags: 要检查的中断标志位掩码组合 IRQ_StatusBitsTypeDef
  * @return Bit_StatusTypeDef: RESET=0 或 SET=!0
  */
Bit_StatusTypeDef HAL_RF_GetIRQFlags(RF_HandleTypeDef *hrf, IRQ_StatusBitsTypeDef Flags)
{
    (void)hrf;
    
    return (Bit_StatusTypeDef)(TRX_IRQ_STATUS & Flags);
}
uint8_t HAL_RF_GetRSSI(RF_HandleTypeDef *hrf)
{
    (void)hrf;
    return (uint8_t)get_XVR_Reg0x13_rssi_ind_o;
}