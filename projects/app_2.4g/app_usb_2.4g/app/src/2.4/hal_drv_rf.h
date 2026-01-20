#ifndef _HAL_DRV_RF_HAL_H_
#define _HAL_DRV_RF_HAL_H_

#include "Types.h"
#include "driver_bk24.h"  //寄存器定义

#ifndef __IO
  #define __IO volatile
#endif

#define pipes_mask(n)      (1U << (n))  // RX Pipe掩码宏:n范围0~5

#define RF_MAX_PACKET_LEN     RX_PLOAD_MAX_WIDTH  /* 最大接收有效载荷宽度 (32字节) */

/* 寄存器位操作宏 */
//工作时,切换模式请使用函数，包含上电使能等操作确保切换成功
#define __HAL_RF_Set_RxMode_Bit()      (TRX_CONFIG |= (1<<0) ) 
#define __HAL_RF_Set_TxMode_Bit()      (TRX_CONFIG &= ~(1<<0) )
#define __HAL_RF_PowerUp()             (TRX_CONFIG |= (1<<1) )
#define __HAL_RF_PowerDown()           (TRX_CONFIG &= ~(1<<1) )  
#define __HAL_RF_Set_CRCO_1Byte()       (TRX_CONFIG &= ~(1<<2) )  
#define __HAL_RF_Set_CRCO_2Byte()       (TRX_CONFIG |= (1<<2) )  
#define __HAL_RF_EN_CRC()               (TRX_CONFIG |= (1<<3) )
#define __HAL_RF_DIS_CRC()              (TRX_CONFIG &= ~(1<<3) )
#define __HAL_RF_EN_IRQ_RX_DR()        (TRX_CONFIG &= ~(1<<6) )
#define __HAL_RF_DIS_IRQ_RX_DR()       (TRX_CONFIG |= (1<<6) )
#define __HAL_RF_EN_IRQ_TX_DS()        (TRX_CONFIG &= ~(1<<5) )
#define __HAL_RF_DIS_IRQ_TX_DS()       (TRX_CONFIG |= (1<<5) )
#define __HAL_RF_EN_IRQ_MAX_RT()       (TRX_CONFIG &= ~(1<<4) )
#define __HAL_RF_DIS_IRQ_MAX_RT()      (TRX_CONFIG |= (1<<4) )

#define __HAL_RF_CHIP_EN()                 (TRX_CE = 0x01)
#define __HAL_RF_CHIP_DIS()                (TRX_CE = 0x00)
/* 命令码*/
#define __HAL_RF_CMD_FLUSH_TXFIFO()               (TRX_CMD = 0xA0)
#define __HAL_RF_CMD_FLUSH_RXFIFO()               (TRX_CMD = 0x80)
#define __HAL_RF_CMD_W_TX_PAYLOAD_NOACK()         (TRX_CMD = 0x68) // 无需ACK发送  命令码
#define __HAL_RF_CMD_W_TX_PAYLOAD()               (TRX_CMD = 0x60) // 带ACK发送    命令码
#define __HAL_RF_CMD_R_RX_PAYLOAD()               (TRX_CMD = 0x40) // 读取接收数据  命令码
#define __HAL_RF_CMD_RX_W_ACK_PAYLOAD(pipes)      (TRX_CMD = 0x68+pipes) //向通道内写入ACK附带的PL 命令码
#define __HAL_RF_CMD_NOP()                        (TRX_CMD = 0x00) // 空操作 命令码

/* Rx,Tx地址宽度设置,__WIDTH__为地址字节数 (3~5) */
#define __HAL_RF_SET_ADDR_WIDTH(__WIDTH__)    (TRX_SETUP_AW = ((__WIDTH__ - 2) & 0x03))

/* 带Ack时，Tx重发延迟和次数,__DELAY__(0~15)->(250us~4000us)和__COUNT__(0~15)->(0~15) */
#define __HAL_RF_TX_RETRY(__DELAY__,__COUNT__)    (TRX_SETUP_RETR = ((__DELAY__ & 0x0F) << 4) | (__COUNT__ & 0x0F))
#define __HAL_RF_SET_RETRY_DELAY(__DELAY__)   (TRX_SETUP_RETR = (TRX_SETUP_RETR & 0x0F) | ((__DELAY__ & 0x0F) << 4))
#define __HAL_RF_SET_RETRY_COUNT(__COUNT__)   (TRX_SETUP_RETR = (TRX_SETUP_RETR & 0xF0) | (__COUNT__ & 0x0F))

/* 允许发送方不带Ack发送 (W_TX_PAYLOAD_NOACK) */
#define __HAL_RF_EN_NOACK()                  (TRX_FEATURE |= (1U << 0))
#define __HAL_RF_DIS_NOACK()                 (TRX_FEATURE &= ~(1U << 0))

/* Rx接收通道使能：传入一个特定通道参数 */
#define __HAL_RF_EN_RXADDR_PIPE(__PIPE__)    (TRX_EN_RXADDR |= (1U << (__PIPE__))) 
#define __HAL_RF_DIS_RXADDR_PIPE(__PIPE__)   (TRX_EN_RXADDR &= ~(1U << (__PIPE__)))

/* Rx自动应答使能 */
#define __HAL_RF_EN_AA_PIPE(__PIPE__)        (TRX_EN_AA |= (1U << (__PIPE__)))
#define __HAL_RF_DIS_AA_PIPE(__PIPE__)       (TRX_EN_AA &= ~(1U << (__PIPE__)))

/* Rx通道动态载荷总开关 */
#define __HAL_RF_EN_DPL()                    (TRX_FEATURE |= (1U << 2))
#define __HAL_RF_DIS_DPL()                   (TRX_FEATURE &= ~(1U << 2))

/* Rx通道动态载荷开关(使能了TRX_FEATURE：bit2即EN_DPL才有用) */
#define __HAL_RF_EN_DPL_PIPE(__PIPE__)       (TRX_DYNPD |= (1U << (__PIPE__)))
#define __HAL_RF_DIS_DPL_PIPE(__PIPE__)      (TRX_DYNPD &= ~(1U << (__PIPE__)))

/* 允许接收方发ACK时附带数据 */
#define __HAL_RF_EN_ACK_PAY()                (TRX_FEATURE |= (1U << 1))
#define __HAL_RF_DIS_ACK_PAY()               (TRX_FEATURE &= ~(1U << 1))

/* 读接收FIFO的有效载荷长度 */
#define __HAL_RF_GET_RX_RPL_WIDTH()                 (TRX_RX_RPL_WIDTH) 


/* 标志位相关 */
#define __HAL_RF_GET_IRQ_Status()             (TRX_IRQ_STATUS)
#define __HAL_RF_CLEAR_IRQ_FLAGS(__FLAGS__)   HAL_RF_ClearIRQFlags(NULL, __FLAGS__)
#define __HAL_RF_GET_IRQ_FLAGS(__FLAGS__)     HAL_RF_GetIRQFlags(NULL, __FLAGS__)

#define __HAL_RF_GET_FIFO_STATUS()            (TRX_FIFO_STATUS)
#define __HAL_RF_GET_FIFO_STATUS_FLAGS(__FLAGS__)  HAL_RF_Get_FifoStatus_Flags(NULL, __FLAGS__)

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
  * @brief  Bit_StatusTypeDef
  */
typedef enum 
{
    RESET = 0,
    SET 
} Bit_StatusTypeDef;

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
  * @brief  RF发送状态机 状态枚举定义
  */
typedef enum 
{
  TX_IDLE,                 /* 空闲状态 */
  TX_BUSY_Tramsmit,        /* 正在发送 */
  TX_Tramsmit_SUCCESS,     /* 发送成功 */
  TX_Tramsmit_FAIL,        /* 发送失败 */
  TX_TIMEOUT              /* 发送超时 */
} HAL_RF_TxFSMTypeDef;


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
  * @brief  RF工作模式枚举
  */
typedef enum 
{
  MODE_TX = 0,
  MODE_RX = 1
   
} RF_ModeTypeDef;

/** 
  * @brief STATUS寄存器：0x40 的中断状态位枚举定义
  */
typedef enum {
    IRQ_RX_DR_MASK    = (1<<6),   /*!< 读=1，表示接收数据就绪  写=1，清除该中断标志 */
    IRQ_TX_DS_MASK    = (1<<5),   /*!< 发送完成中断,1触发，写1，清除该中断标志 */
    IRQ_MAX_RT_MASK   = (1<<4),   /*!< 最大重传次数中断，1触发，写1，清除该中断标志 */
    RX_P_NO  = (1<<1|1<<2|1<<3),   /*!< 指示当前接收FIFO中的数据来自哪个pipes 111表示没数据 */
    TX_FULL = (1<<0)    /*!< TX FIFO满状态标志 */
} IRQ_StatusBitsTypeDef;

/** 
  * @brief FIFO状态位枚举定义（FIFO_STATUS寄存器：0x43）
  */
typedef enum {
    RF_TX_REUSE  = (1 << 6),   /*!< TX重复发送标志 */
    RF_TX_FULL   = (1 << 5),   /*!< TX FIFO满状态标志 ==1 ->FULL*/
    RF_TX_EMPTY  = (1 << 4),   /*!< TX FIFO空状态标志 ==1 ->EMPTY */
    RF_RX_FULL   = (1 << 1),   /*!< RX FIFO满状态标志 ==1 ->FULL */
    RF_RX_EMPTY  = (1 << 0)    /*!< RX FIFO空状态标志 ==1->EMPTY */
} RF_FIFOStatusBitsTypeDef;


/** 
  * @brief  RF接收通道配置结构体
  */
typedef struct {
    uint8_t  PipeNum;          /*!< 通道号 0~5 */
    uint8_t  Enable;           // 该pipe是否使能
    uint8_t  AutoAck;          // 是否使能自动应答
    uint8_t  EnDynamicPayload;   // 是否使能动态包长
    uint8_t  PayloadWidth;       // 固定包长;EnDynamicPayload无效时通道的固定包长值
    uint8_t  Support_Payload_Attach_ACK; // 是否支持接收方发ACK时附带数据
    uint32_t Address[5];          // pipe的地址（pipe0/1为全地址，pipe2-5只用最低字节）
} RF_RxPipesTypeDef;

/** 
  * @brief  RF协议配置结构体
  */
typedef struct {
    uint8_t    AddressWidth;     /*!< 全局地址宽度: 3, 4 或 5 字节 */
    
    uint32_t   TxAddress[5];     /*!< 发送地址 (本机作为PTX发包时的目标地址) */
    uint8_t    Support_NoAck;            /*!< 是否支持无ACK发送命令 1==支持 (W_TX_PAYLOAD_NOACK) */
    uint8_t    AutoRetransmitCount;    /*!< 自动重传次数 (0~15, 0表示禁用重传) */
    uint8_t    AutoRetransmitDelay;    /*!< 自动重传延迟 (单位: 250μs, 0~15 对应 250μs~4000μs) */

    RF_RxPipesTypeDef RxPipes[6];  /*!< 6个接收通道配置结构体数组:注意pipe0的地址必须和tx地址相同才能使用ACK */
    
} RF_ProtocolTypeDef;


/** 
  * @brief 中断事件下标索引
  */
typedef enum {
    IRQ_Event_RX_DR    = 0,   
    IRQ_Event_TX_DS    = 1,   
    IRQ_Event_MAX_RT   = 2,  
} IRQEvent_Index_E;


/** 
  * @brief  RF中断管理结构体
  */
typedef struct {
    uint8_t enable;
    /* 处理函数指针 */  
    void (*user_cb)(void);
} RF_IRQManager_TypeDef;

typedef struct {
    /*函数指针:获取当前系统时间，单位ms */  
    uint32_t (*GetSysTimeMs)(void);
    
    /* 发送超时相关标志位 */
    uint32_t  Tx_start_time;
    uint32_t  Tx_TimeOut; //超时时间
    uint8_t  Tx_TimeOut_flag; 
    uint32_t Tx_Timeout_cnt;

} RF_TimeManager_TypeDef;

/** 
  * @brief  RF初始化配置结构体
  *       
  */
typedef struct {
    RF_ModeTypeDef      Mode;           /*!< 工作模式: 接收或发送 */
    RF_DataRateTypeDef  DataRate;       /*!< 数据速率 */
    RF_TxPowerTypeDef   TxPower;        /*!< 发射功率 */
    uint8_t             Channel;        /*!< 初始频段 channel */
    /* 协议配置 */
    RF_ProtocolTypeDef  Protocol;       /*!< 协议相关配置 */
    
    /* 中断 */
    RF_IRQManager_TypeDef IRQ_Manager[3];  /*!< 中断管理,下标传入IRQEvent_Index_E */         
        
} RF_ConfgTypeDef;


/** 
  * @brief  RF 句柄结构体
  *         包含实例上下文、配置信息和运行时状态
  */
typedef struct 
{
    RF_ConfgTypeDef           Params;         /*!< 通信参数配置 */
    RF_TimeManager_TypeDef    TimeManager;    /*!< 时间管理 */
    __IO uint8_t             *pTxBuff;        /*!< 发送缓存指针 */
    __IO uint8_t              RxBuff[32];     /*!< 接收缓存数组 */
    __IO uint8_t              RxBuff_valid;     /*!< 接收缓存有效标志 */
    HAL_RF_StateTypeDef       State;          /*!< 驱动运行状态 */
    __IO uint8_t              ErrorCode;      /*!< 错误代码 */
    
    /*运行时参数 */
    __IO HAL_RF_TxFSMTypeDef   TxState;     /*!< 发送状态 */
    __IO RF_ModeTypeDef          Cur_Mode;    /*!< 当前工作模式 */

    uint8_t                 CurrentChannel; 
    uint8_t                 LastRxLen;      
} RF_HandleTypeDef;



/* 初始化 */
HAL_StatusTypeDef HAL_RF_Init(RF_HandleTypeDef* hrf,RF_ConfgTypeDef *Init);
/* 注册获取系统时间的函数,用于超时管理 */
HAL_StatusTypeDef HAL_RF_TimeManager_register(RF_HandleTypeDef *hrf, uint32_t (*GetSysTimeMs)(void));

HAL_StatusTypeDef HAL_RF_MspInit(RF_HandleTypeDef *hrf); //弱定义，用于GPIO/SPI底层初始化



/* 数据收发函数 */
HAL_RF_StateTypeDef HAL_RF_Transmit_ACK(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);

HAL_StatusTypeDef HAL_RF_Transmit_IT(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);
HAL_StatusTypeDef HAL_RF_Receive_IT(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);


HAL_RF_StateTypeDef HAL_RF_Transmit_NoACK(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);
HAL_RF_StateTypeDef HAL_RF_Receive(RF_HandleTypeDef *hrf, uint8_t *pData, uint8_t Size);
void HAL_RF_Attach_PL2ACK(uint8_t pipes, uint8_t *pBuf, uint8_t len);


/* 中断处理函数, 在intc.c的intc_irq/intc_fiq中调用，自动判断中断类型并处理中断事件回调 */
void HAL_RF_IRQ_Handler(RF_HandleTypeDef *hrf);


/* 控制函数 */
void HAL_RF_SetChannel(RF_HandleTypeDef *hrf, uint8_t channel);
void HAL_RF_GetChannel(RF_HandleTypeDef *hrf, uint8_t channel);

void HAL_RF_SetTxAddress(RF_HandleTypeDef *hrf, uint32_t *dev_addr);
void HAL_RF_GetTxAddress(RF_HandleTypeDef *hrf, uint32_t *dev_addr);

void HAL_RF_SetRxAddress(RF_HandleTypeDef *hrf, uint8_t pipe, uint32_t *dev_addr);
void HAL_RF_GetRxAddress(RF_HandleTypeDef *hrf, uint8_t pipe, uint32_t *dev_addr);

//注意，切换模式不可只改变寄存器位，还需PowerUp以及拉低CE
void HAL_RF_SetRxMode(RF_HandleTypeDef *hrf);
void HAL_RF_SetTxMode(RF_HandleTypeDef *hrf);


void HAL_RF_ClearIRQFlags(RF_HandleTypeDef *hrf, IRQ_StatusBitsTypeDef _Flags);
uint8_t HAL_RF_GetIRQFlags(RF_HandleTypeDef *hrf, IRQ_StatusBitsTypeDef IRQ_Flags);

uint8_t HAL_RF_Get_FifoStatus_Flags(RF_HandleTypeDef *hrf, RF_FIFOStatusBitsTypeDef Flags);
/* 状态查询 */
HAL_RF_StateTypeDef HAL_RF_GetState(RF_HandleTypeDef *hrf);
uint8_t HAL_RF_GetRSSI(RF_HandleTypeDef *hrf);


#endif /* _DRIVER_RF_HAL_H_ */
