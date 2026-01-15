#include <stdio.h>
#include "driver_bk24.h"
#include "driver_timer.h"
#include "bk3633_reglist.h"
#include "user_config.h"



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

static void RF_Write_fifo(uint8_t *pBuf, uint8_t bytes)
{
    uint8_t i;
    __HAL_RF_CMD_W_TX_PAYLOAD();
    for(i=0; i<bytes; i++)
    {
        TRX_FIFO = pBuf[i];
    }
    __HAL_RF_CMD_NOP();
}
static void RF_Write_fifo_NoAck(uint8_t *pBuf, uint8_t bytes)
{
    uint8_t i;
    __HAL_RF_CMD_W_TX_PAYLOAD_NOACK();
    for(i=0; i<bytes; i++)
    {
        TRX_FIFO = pBuf[i];
    }
    __HAL_RF_CMD_NOP();
}

static void RF_Read_fifo(uint8_t *pBuf, uint8_t bytes)
{
    uint8_t i;
    __HAL_RF_CMD_R_RX_PAYLOAD();
    for(i=0; i<bytes; i++)
    {
        pBuf[i] = TRX_FIFO;
    }
    __HAL_RF_CMD_NOP();
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
    
    //清除外设电源掉电位 (Enable Peripheral Clock)
    clrf_SYS_Reg0x3_bk24_pwd;

    /* 配置默认模拟寄存器参数 (原厂Magic Numbers) */
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
 
    /* 地址宽度 */
    uint8_t tmp_addr_width=Init->Protocol.AddressWidth;
    if(tmp_addr_width < 3 || tmp_addr_width > 5)
        return HAL_ERROR; // 地址宽度无效
    __HAL_RF_SET_ADDR_WIDTH(tmp_addr_width);

   
    RF_MemCpy(&TRX_TX_ADDR_0, (uint32_t*)Init->Protocol.TxAddress, tmp_addr_width);   // TX 发送地址
    Init->Protocol.Support_NoAck ? __HAL_RF_EN_NOACK() : __HAL_RF_DIS_NOACK();    // 是否支持无ACK发送命令
    __HAL_RF_TX_RETRY(Init->Protocol.AutoRetransmitDelay, Init->Protocol.AutoRetransmitCount); //自动重传设置

    /* 接收pipes设置 */
    for(uint8_t i=0; i<6; i++)
    {
        Init->Protocol.RxPipes[i].Enable ? __HAL_RF_EN_RXADDR_PIPE(i) : __HAL_RF_DIS_RXADDR_PIPE(i);
        Init->Protocol.RxPipes[i].AutoAck ? __HAL_RF_EN_AA_PIPE(i) : __HAL_RF_DIS_AA_PIPE(i);
        
        //动态包长依赖 EN_DPL and ENAA
        if(Init->Protocol.RxPipes[i].EnDynamicPayload){
            __HAL_RF_EN_DPL();      //EN_DPL
            __HAL_RF_EN_AA_PIPE(i); //EN_AA
            __HAL_RF_EN_DPL_PIPE(i);//EN_DPL_PIPE
        }
        else{
            __HAL_RF_DIS_DPL_PIPE(i);
        }

        /* 无论是否是动态包长，都设置一个固定包长;启用动态包长时这些寄存器无效，不影响运行 */
        uint8_t PW = Init->Protocol.RxPipes[i].PayloadWidth;
        if(PW>32) PW=32; //最大32字节
        switch(i)
        {
            case 0: TRX_RX_PW_P0 = PW; break;
            case 1: TRX_RX_PW_P1 = PW; break;
            case 2: TRX_RX_PW_P2 = PW; break;
            case 3: TRX_RX_PW_P3 = PW; break;
            case 4: TRX_RX_PW_P4 = PW; break;
            case 5: TRX_RX_PW_P5 = PW; break;
            default: break;
        }

        Init->Protocol.RxPipes[i].Support_Payload_Attach_ACK ? __HAL_RF_EN_ACK_PAY() : __HAL_RF_DIS_ACK_PAY();
    }
    RF_MemCpy(&TRX_RX_ADDR_P0_0, (uint32_t*)Init->Protocol.RxPipes[0].Address, tmp_addr_width);
    RF_MemCpy(&TRX_RX_ADDR_P1_0, (uint32_t*)Init->Protocol.RxPipes[1].Address, tmp_addr_width);
    TRX_RX_ADDR_P2 = Init->Protocol.RxPipes[2].Address[0];
    TRX_RX_ADDR_P3 = Init->Protocol.RxPipes[3].Address[0];
    TRX_RX_ADDR_P4 = Init->Protocol.RxPipes[4].Address[0];
    TRX_RX_ADDR_P5 = Init->Protocol.RxPipes[5].Address[0];


    /* ---------------- 射频物理层配置 ---------------- */
    //信道
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

    uart_printf("set bps success\r\n", __FILE__, __LINE__);

    xvr_reg_initial_24(); // 配置 XVR 寄存器

    __HAL_RF_PowerUp();
    __HAL_RF_CMD_FLUSH_RXFIFO();
    __HAL_RF_CMD_FLUSH_TXFIFO();
    __HAL_RF_CLEAR_IRQ_FLAGS((IRQ_RX_DR|IRQ_TX_DS|IRQ_MAX_RT|RX_P_NO));

    // 3. 初始化 RF_HandleTypeDef 结构体
    hrf->Params = *Init;
    hrf->State = HAL_RF_STATE_READY;

    return HAL_OK;
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
    uint8_t status;
    uint8_t isTimeout = 0;      // 是否超时标志
    uint32_t txWaitCounter = 0; // 发送等待计数器（用于超时判断）
    const uint16_t MAX_TX_TIMEOUT = 1000; // 最大等待时间，单位10us (1000*10us=10ms)

    if (hrf == NULL || pData == NULL || Size == 0) return HAL_RF_STATE_ERROR;

    //进入发送模式
    HAL_RF_SetTxMode(hrf);

    //清空FIFO和中断
    __HAL_RF_CMD_FLUSH_RXFIFO();
    __HAL_RF_CMD_FLUSH_TXFIFO();
    __HAL_RF_CLEAR_IRQ_FLAGS(IRQ_RX_DR | IRQ_TX_DS | IRQ_MAX_RT | RX_P_NO);

    __HAL_RF_CHIP_EN();
    RF_Write_fifo(pData, Size);

    while(__HAL_RF_GET_IRQ_FLAGS(IRQ_TX_DS | IRQ_MAX_RT) == 0){ //等待发送完成或最大重传中断
        RF_DelayUs(10);
        txWaitCounter++;
        if (txWaitCounter > MAX_TX_TIMEOUT) {
            isTimeout = 1;
            break; 
        }
    } 

    if (__HAL_RF_GET_IRQ_FLAGS(IRQ_RX_DR)) {
        __HAL_RF_CMD_FLUSH_RXFIFO();
        __HAL_RF_CLEAR_IRQ_FLAGS(IRQ_RX_DR);
        hrf->State = HAL_RF_STATE_READY;
        return HAL_RF_STATE_READY; // 发送并收到ACK
    }

    if (__HAL_RF_GET_IRQ_FLAGS(IRQ_TX_DS)) {
        __HAL_RF_CMD_FLUSH_TXFIFO();
        __HAL_RF_CLEAR_IRQ_FLAGS(IRQ_TX_DS);
        hrf->State = HAL_RF_STATE_READY;
        return HAL_RF_STATE_READY; // 发送成功
    }

    if ((__HAL_RF_GET_IRQ_FLAGS(IRQ_MAX_RT)) || isTimeout) {
        __HAL_RF_CMD_FLUSH_RXFIFO();
        __HAL_RF_CMD_FLUSH_TXFIFO();
        __HAL_RF_PowerDown();
        __HAL_RF_PowerUp();
        __HAL_RF_CLEAR_IRQ_FLAGS(IRQ_MAX_RT);
        hrf->State = HAL_RF_STATE_TIMEOUT;
        return HAL_RF_STATE_TIMEOUT; // 超时或最大重传，重启芯片
    }

    __HAL_RF_CHIP_DIS();
    hrf->State = HAL_RF_STATE_ERROR;
    return HAL_RF_STATE_ERROR;
}
/**  
  * @brief  接收数据查询
  * @param  pData: 接收缓冲区
  * @param  Size:  期望读取长度 (如果 FIFO 数据小于此长度，将只读取实际长度)
  * @return HAL_RF_STATE_READY (有数据) 或 HAL_RF_STATE_BUSY_RX (无数据)
  */
HAL_RF_StateTypeDef HAL_RF_Receive(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size)
{
    uint8_t status;
    uint8_t fifo_status;
    uint8_t len;
    uint8_t rx_cnt = 0;

    if (hrf == NULL || pData == NULL || Size == 0) return HAL_RF_STATE_ERROR;

    status = __HAL_RF_GET_IRQ_Status();

    if (__HAL_RF_GET_IRQ_FLAGS(IRQ_RX_DR)){  //接收标志位置1，接收FIFO中的数据就绪
        do
        {
            len = TRX_RX_RPL_WIDTH;
            if (len <= RF_MAX_PACKET_LEN)
            {
                // 读取接收数据
                RF_Read_fifo(pData, len);
                rx_cnt++;
            }
            else
            {
                __HAL_RF_CMD_FLUSH_RXFIFO();
                break;
            }
            fifo_status = __HAL_RF_GET_IRQ_Status();
        }
        while (__HAL_RF_GET_FIFO_STATUS_FLAGS(RF_RX_EMPTY)!=0); // FIFO非空则继续

        // 清除中断标志
        __HAL_RF_CLEAR_IRQ_FLAGS(IRQ_RX_DR);

        hrf->State = HAL_RF_STATE_READY;
        return HAL_RF_STATE_READY;
    }

    return HAL_RF_STATE_BUSY_RX; // 没有数据
}

/* 控制函数实现 */
void HAL_RF_Attach_PL2ACK(uint8_t pipes, uint8_t *pBuf, uint8_t len) 
{
    uint8_t i;
    __HAL_RF_CMD_RX_W_ACK_PAYLOAD(pipes);
    for(i = 0; i < len; i++) 
    {
        TRX_FIFO = pBuf[i];
    }
    __HAL_RF_CMD_NOP();
}



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
  * @brief  设置为接收模式
  * @param  hrf: RF 句柄
  * @param  dev_addr: 发送地址指针
  */
void HAL_RF_SetRxMode(RF_HandleTypeDef *hrf)
{
    hrf->Params.Mode = MODE_RX;
    __HAL_RF_PowerUp();
    __HAL_RF_CMD_FLUSH_RXFIFO();

    __HAL_RF_CHIP_DIS();
    __HAL_RF_Set_RxMode_Bit() ;
    __HAL_RF_CHIP_EN();

    hrf->State = HAL_RF_STATE_BUSY_RX;
}

/**
  * @brief  设置为发送模式
  * @param  hrf: RF 句柄
  */
void HAL_RF_SetTxMode(RF_HandleTypeDef *hrf)
{
    hrf->Params.Mode = MODE_TX;

    __HAL_RF_PowerUp();
    __HAL_RF_CMD_FLUSH_TXFIFO();

    __HAL_RF_CHIP_DIS();
    __HAL_RF_Set_TxMode_Bit();
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
  * @return 注意返回的是0或者非0值,非零值不一定是1
  */
uint8_t HAL_RF_GetIRQFlags(RF_HandleTypeDef *hrf, IRQ_StatusBitsTypeDef IRQ_Flags)
{
    (void)hrf;
    
    return (uint8_t)((TRX_IRQ_STATUS & (IRQ_Flags)));
}

/**
  * @brief  检查FIFO状态标志位是否置位
  * @param  hrf: RF 句柄
  * @param  Flags: 要检查的FIFO状态标志位掩码组合 RF_FIFOStatusBitsTypeDef
  * @return 注意返回的是0或者非0值,非零值不一定是1
  */
uint8_t HAL_RF_Get_FifoStatus_Flags(RF_HandleTypeDef *hrf, RF_FIFOStatusBitsTypeDef Flags)
{
    (void)hrf;

    return (uint8_t)(TRX_FIFO_STATUS & (Flags));
}

uint8_t HAL_RF_GetRSSI(RF_HandleTypeDef *hrf)
{
    (void)hrf;
    return (uint8_t)get_XVR_Reg0x13_rssi_ind_o;
}