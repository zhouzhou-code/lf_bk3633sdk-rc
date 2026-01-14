#ifndef _DRIVER_RF_H_
#define _DRIVER_RF_H_
//#include "BK_System.h"
#include "Types.h"
#include "driver_bk24.h"
// Interrupt status register bits-mask
#define STATUS_RX_TX_MAX                        0x7e  // 0x70
#define STATUS_RX_TX_MAX_TXFULL                 0x71
#define STATUS_RX_DR                            (1<<6) //接收到数据存入rx fifo，则置位
#define STATUS_TX_DS                            (1<<5) //数据发送完成(收到ACK)，则置位
#define STATUS_MAX_RT                           (1<<4) //达到最大重发次数，则置位
#define STATUS_TX_FULL                          (1<<0) //TX FIFO满，则置位
#define STATUS_TX                               (STATUS_TX_DS | STATUS_MAX_RT)   //bit5|bit4 重传时使用，判断是否发送完成或达到最大重传次数
#define STATUS_RX_EMPTY                         (1<<3|1<<2|1<<1) //bit[3:1] 

#define RF_CHIP_ENABLE                          TRX_CE = 0x01
#define RF_CHIP_DISABLE                         TRX_CE = 0x00
#define RF_POWER_UP                             PowerUp_Rf()

#define RF_POWER_DOWN                           PowerDown_Rf()
#define RF_CMD_NOP
#define RF_CMD_FLUSH_TX                         { TRX_CMD = 0xA0; }
#define RF_CMD_FLUSH_RX                         { TRX_CMD = 0x80; }
#define RF_CMD_CLEAR_STATUS_RX                  { TRX_IRQ_STATUS = STATUS_RX_DR; }
#define RF_CMD_CLEAR_STATUS_TX                  { TRX_IRQ_STATUS = STATUS_TX_DS; }
#define RF_CMD_CLEAR_STATUS_MAX                 { TRX_IRQ_STATUS = STATUS_MAX_RT; }
#define RF_CMD_CLEAR_STATUS_ALL                 { TRX_IRQ_STATUS = STATUS_RX_TX_MAX; }
#define MAX_PACKET_LEN      32

extern uint8 RF_CHANNEL_TABLE[4][4];

extern uint8 uRXDataLens;
extern uint8 flag_test;
extern uint8 channel_search;

extern uint32 P0_Address[5];
extern uint32 rf_address_tx_pub[5];

void PowerUp_Rf(void);   //模块电源上电
void PowerDown_Rf(void); //模块电源断电
void SwitchToRxMode(void); //切换到接收模式
void SwitchToTxMode(void); //切换到发送模式
void ChangeTxPipe(uint8_t pipe); //切换发送管道

/* 基本收发接口 数据读写硬件FIFO */
void R_RX_PAYLOAD(uint8_t *pBuf, uint8_t bytes); //读取接收FIFO的数据到pBuf
void W_TX_PAYLOAD(uint8_t *pBuf, uint8_t bytes); //将pBuf数据写入发送FIFO
void W_TX_PAYLOAD_NOACK(uint8_t *pBuf, uint8_t bytes); //写入发送数据，无需应答
void W_ACK_PAYLOAD(uint8_t *pBuf, uint8_t bytes, uint8_t pipe); //写入应答数据

void Rf_Init(void); //初始化RF模块
void memcpy_2461(volatile uint32 *p, uint32 *val, uint8 len); //内存拷贝函数
void SetDataRate(int data_rate);     //设置数据传输速率

void driver_rf_channel_switch(void); //自动跳频
void driver_set_rf_channel(uint8_t uArray, uint8_t uIndx); //设置通信频道
uint32_t driver_rf_data_send(uint8_t len);
uint32_t driver_rf_data_retrans(void);
void driver_rf_maxRT_clear(void);
uint8_t driver_rf_data_send_noACK(uint8_t len);
uint8_t driver_rf_data_send_ACK(uint8_t len);
void driver_rf_receive_package(void);
void driver_rf_ouput_power_value_set(uint8_t power_level);
uint8_t get_rf_rssi(void);

#endif
