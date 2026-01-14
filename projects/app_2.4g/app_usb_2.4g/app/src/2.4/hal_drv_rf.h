#ifndef _HAL_DRV_RF_HAL_H_
#define _HAL_DRV_RF_HAL_H_

#include "Types.h"
#include "driver_bk24.h"  //寄存器定义

#ifndef __IO
  #define __IO volatile
#endif

#define rx_pipes(n)      (1U << (n))  // RX Pipe掩码宏

#define RF_MAX_PACKET_LEN     RX_PLOAD_MAX_WIDTH  /* 最大接收有效载荷宽度 (32字节) */

/* 操作宏 */
#define __HAL_RF_PowerUp()                    (TRX_CONFIG |= (1<<1) )
#define __HAL_RF_PowerDown()                  (TRX_CONFIG &= ~(1<<1) )  
#define __HAL_RF_ChipEnable()                 (TRX_CE = 0x01)
#define __HAL_RF_ChipDisable()                (TRX_CE = 0x00)
#define __HAL_RF_FLUSH_TXFIFO()               { TRX_CMD = 0xA0 }
#define __HAL_RF_FLUSH_RXFIFO()               { TRX_CMD = 0x80 }
#define __HAL_RF_CLEAR_IRQ_FLAGS(__FLAGS__)   HAL_RF_ClearIRQFlags(NULL, __FLAGS__)
#define __HAL_RF_GET_IRQ_FLAGS()              (TRX_IRQ_STATUS)

#define __HAL_EN_AA_PIPE(__PIPE__)        (TRX_EN_AA |= (1U << (__PIPE__)))
#define __HAL_DIS_AA_PIPE(__PIPE__)       (TRX_EN_AA &= ~(1U << (__PIPE__)))
#define __HAL_EN_RXADDR_PIPE(__PIPE__)    (TRX_EN_RXADDR |= (1U << (__PIPE__)))
#define __HAL_DIS_RXADDR_PIPE(__PIPE__)   (TRX_EN_RXADDR &= ~(1U << (__PIPE__)))


/** 
  * @brief  HAL 通用状态枚举
  */
typedef enum 
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/** 
  * @brief  RF 驱动状态枚举定义
  */
typedef enum 
{
    HAL_RF_STATE_RESET      = 0x00U,    /*!< 尚未初始化 */
    HAL_RF_STATE_READY      = 0x01U,    /*!< 初始化完成，空闲 */
    HAL_RF_STATE_BUSY       = 0x02U,    /*!< 忙碌 (通用) */
    HAL_RF_STATE_BUSY_TX    = 0x12U,    /*!< 正在发送 */
    HAL_RF_STATE_BUSY_RX    = 0x22U,    /*!< 正在接收 */
    HAL_RF_STATE_TIMEOUT    = 0x03U,    /*!< 超时 */
    HAL_RF_STATE_ERROR      = 0x04U     /*!< 发生错误 */
} HAL_RF_StateTypeDef;

/** 
  * @brief  RF数据速率枚举
  */
typedef enum 
{
    BPS_250K = 0,
    BPS_1M=1,
    BPS_2M=2
} RF_DataRateTypeDef;

/** 
  * @brief  RF发射功率枚举
  */
typedef enum 
{
    RF_TX_POWER_N18dBm = 0,  // -18dBm
    RF_TX_POWER_N12dBm,      // -12dBm
    RF_TX_POWER_N6dBm,       // -6dBm
    RF_TX_POWER_0dBm,        // 0dBm
} RF_TxPowerTypeDef;


/** 
  * @brief  RF 地址配置结构体
  */
typedef struct
{
    uint8_t   AddressWidth;     /*!< 全局地址宽度: 3, 4 或 5 字节 */
    uint8_t   TxAddress[5];     /*!< 发送地址 (本机作为PTX发包时的目标地址) */

    /* 接收通道地址配置 */
    /* 注意：Pipe 2-5 的高位地址共用 Pipe 1 的高位 */
    uint8_t   RxAddrP0[5];      /*!< 通道0接收地址 (若使用ACK，则RxAddrP0等于TxAddress) */
    uint8_t   RxAddrP1[5];      /*!< 通道1接收地址 */
    uint8_t   RxAddrP2;         /*!< 通道2接收地址 (仅LSB:最低字节和P1共用高四位地址) */
    uint8_t   RxAddrP3;         /*!< 通道3接收地址 (仅LSB:最低字节) */
    uint8_t   RxAddrP4;         /*!< 通道4接收地址 (仅LSB:最低字节) */
    uint8_t   RxAddrP5;         /*!< 通道5接收地址 (仅LSB:最低字节) */
} RF_AddressTypeDef;

/** 
  * @brief  RF工作模式枚举
  */
typedef enum 
{
  MODE_TX = 0,
  MODE_RX = 1
   
} RF_ModeTypeDef;


/** 
  * @brief  RF协议配置结构体
  */
typedef struct
{

    bool alloc_noAck;                /*!< 是否支持无ACK发送命令 (W_TX_PAYLOAD_NOACK) */

    uint8_t   DynamicPayloadEnable;   /*!< 全局动态包长使能 (0:禁用, 1:启用)
                                           - 若启用: 硬件自动处理包长，PayloadWidth无效
                                           - 若禁用: 必须设置 PayloadWidth 作为自动包长*/
                                           
    uint8_t   PayloadWidth;           /*!< 固定包长 (1~32字节)
                                           仅在 DynamicPayloadEnable=0 时有效 */
                                           
    uint8_t   RxPipesEn_Mask;          /*!< 接收通道使能掩码bit[5:0]->管道5~0，对应位1启用
                                          eg:rx_pipes(0) | rx_pipes(1) 启用管道0和1
                                          */
    
    uint8_t   RxPipesAutoAckEn_Mask;       /*!< 自动应答通道使能掩码bit[5:0]->管道5~0，对应位1启用
                                          eg:rx_pipes(0) | rx_pipes(1) 启用管道0和1的自动应答
                                            */
    /* Tx自动重传 */
    uint8_t   AutoRetransmitCount;    /*!< 自动重传次数 (0~15, 0表示禁用重传) */
    uint8_t   AutoRetransmitDelay;    /*!< 自动重传延迟 (单位: 250μs, 0~15 对应 250μs~4000μs) */

} RF_ProtocolTypeDef;

/** 
  * @brief  RF初始化配置结构体
  *       
  */
typedef struct 
{
    RF_ModeTypeDef Mode;               /*!< 工作模式: 接收或发送 */
    RF_DataRateTypeDef  DataRate;       /*!< 数据速率 */
    RF_TxPowerTypeDef   TxPower;        /*!< 发射功率 */
    uint8_t             Channel;        /*!< 初始频段 channel */

    /* 地址配置 */
    RF_AddressTypeDef   Address;        /*!< 配置地址宽度与各接收通道地址 */
    /* 协议配置 */
    RF_ProtocolTypeDef  Protocol;       /*!< 协议相关配置 */

} RF_ConfgTypeDef;


/** 
  * @brief  RF 句柄结构体
  *         包含实例上下文、配置信息和运行时状态
  */
typedef struct 
{
    RF_ConfgTypeDef           Init;           /*!< 通信参数配置 */
    __IO uint8_t             *pTxBuff;       /*!< 发送缓存指针 */
    __IO uint8_t             *pRxBuff;       /*!< 接收缓存指针 */
    __IO HAL_RF_StateTypeDef State;          /*!< 驱动运行状态 */
    __IO uint8_t             ErrorCode;      /*!< 错误代码 */
    

    uint8_t                 CurrentChannel; 
    uint8_t                 LastRxLen;      
} RF_HandleTypeDef;


/** 
  * @brief  中断状态位枚举定义
  */
typedef enum 
{
    RX_DR    = (1<<6),  /*!< 读=1，表示接收数据就绪  写=1，清除该中断标志 */
    TX_DS    = (1<<5),  /*!< 发送完成中断,1触发，写1，清除该中断标志 */
    MAX_RT   = (1<<4),  /*!< 最大重传次数中断，1触发，写1，清除该中断标志 */
    RX_P_NO  = (1<<1|1<<2|1<<3),   /*!< 指示当前接收FIFO中的数据来自哪个pipes */
    TX_FULL  = (1<<0)   /*!< TX FIFO满中断 */
    
} IRQ_StatusBitsTypeDef;

HAL_StatusTypeDef HAL_RF_StructInit(RF_InitTypeDef *Init);
/* 初始化与反初始化 */
HAL_StatusTypeDef HAL_RF_Init(RF_HandleTypeDef *hrf);
HAL_StatusTypeDef HAL_RF_DeInit(RF_HandleTypeDef *hrf);
HAL_StatusTypeDef HAL_RF_MspInit(RF_HandleTypeDef *hrf); //弱定义，用于GPIO/SPI底层初始化

/* 数据收发函数 */
HAL_RF_StateTypeDef HAL_RF_Transmit(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);
HAL_RF_StateTypeDef HAL_RF_Transmit_NoAck(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);
HAL_RF_StateTypeDef HAL_RF_Receive(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);

/* 控制函数 */
void HAL_RF_SetChannel(RF_HandleTypeDef *hrf, uint8_t channel);
void HAL_RF_GetChannel(RF_HandleTypeDef *hrf, uint8_t channel);

void HAL_RF_SetTxAddress(RF_HandleTypeDef *hrf, uint32_t *dev_addr);
void HAL_RF_GetTxAddress(RF_HandleTypeDef *hrf, uint32_t *dev_addr);

void HAL_RF_SetRxAddress(RF_HandleTypeDef *hrf, uint8_t pipe, uint32_t *dev_addr);
void HAL_RF_GetRxAddress(RF_HandleTypeDef *hrf, uint8_t pipe, uint32_t *dev_addr);

void HAL_RF_SetRxMode(RF_HandleTypeDef *hrf);
void HAL_RF_SetTxMode(RF_HandleTypeDef *hrf);

void HAL_RF_ClearIRQFlags(RF_HandleTypeDef *hrf, IRQ_StatusBitsTypeDef _Flags);

/* 状态查询 */
HAL_RF_StateTypeDef HAL_RF_GetState(RF_HandleTypeDef *hrf);
uint8_t HAL_RF_GetRSSI(RF_HandleTypeDef *hrf);


#endif /* _DRIVER_RF_HAL_H_ */