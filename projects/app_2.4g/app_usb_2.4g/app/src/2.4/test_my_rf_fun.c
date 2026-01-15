#include "app.h"
#include "bk3633_reglist.h"
#include "icu.h"
#include "driver_timer.h"
#include <string.h>

#include "hal_drv_rf.h"

#define UART_PRINTF    uart_printf
int uart_printf(const char *fmt,...);



#if 0
void rf_simple_init(void)
{
    Rf_Init();           // 射频模块初始化
    SetDataRate(2);      // 设置速率为2Mbps

    uart_printf("in 2.4 mode1===============\r\n");
    xvr_reg_initial_24();
    PowerUp_Rf();      // 射频模块上电
    RF_CHIP_ENABLE;    // 使能射频芯片
    RF_CMD_FLUSH_TX;   // 清空发送FIFO
}

void bk24_send_data(void)
{
    static uint32_t packet_count = 0;
    uint8_t rf_fifo_data[32];

    for(uint8_t i = 0; i < MAX_PACKET_LEN; i++) 
    {
        rf_fifo_data[i] = i;
    }

    SwitchToTxMode();

    while(1)
    {
        RF_CMD_FLUSH_TX;              // 清空TX FIFO
        RF_CMD_CLEAR_STATUS_ALL;      // 清除中断标志
        
        // 写入数据并发送
        W_TX_PAYLOAD(rf_fifo_data, MAX_PACKET_LEN);
        RF_CHIP_ENABLE;
        Delay_us(15);
        RF_CHIP_DISABLE;
        
        packet_count++;
        
        // 简单打印，每100包打印一次
        if(packet_count % 100 == 0) 
        {
            uart_printf("send %ld packet\n", packet_count);
        }
        
        // 简单延时，控制发送速率
        Delay_us(100);
    }
}

void rf_simple_receive()
{
    SwitchToRxMode();
    uint8_t rec[32];
    uint16_t count_packet; 
    uint16_t count_printf; 
    while(1)
    {
        if (TRX_IRQ_STATUS & STATUS_RX_DR)
        {
            R_RX_PAYLOAD(rec,32);
            RF_CMD_CLEAR_STATUS_RX;
            count_packet++;
            uart_printf("recieve %ld packet: ", count_packet);
            // 按照十进制打印接收到的数据
            for(uint8_t i = 0; i < 32; i++)
            {
                uart_printf("%d ", rec[i]);
            }
            uart_printf("\n");
        }
        else
        {
            count_printf++;
            if(count_printf % 10000 == 0)
            {
                uart_printf("not received\n");
            }

        }

    }

}

#endif

RF_ConfgTypeDef Init_S_normal=
{
    .Mode = MODE_TX,
    .DataRate = BPS_2M,
    .TxPower = RF_TX_POWER_0dBm,
    .Channel = 0x05, // 频道5

    .Address =
    {
        .AddressWidth = 5,
        .TxAddress = {0x10, 0x11, 0x36, 0x0, 0x0},
        .RxAddrP0 = {0x10, 0x11, 0x36, 0x0, 0x0},
        .RxAddrP1 = {0x10, 0x56, 0x24, 0xCD, 0x78},
        .RxAddrP2 = 0x11,
        .RxAddrP3 = 0x12,
        .RxAddrP4 = 0x13,
        .RxAddrP5 = 0x14
    },

    .Protocol =
    {
        .alloc_noAck = 1,             
        .DynamicPayloadEnable = 1,       //启用动态包长                     
        .PayloadWidth = 32,              // 固定包长 32字节 (仅在 DynamicPayloadEnable=0 时有效)
        .RxPipesEn_Mask = 0x03,          // 启用所有pipes的接收
        .RxPipesAutoAckEn_Mask = 0x03,   // 启用所有pipes的自动应答
        .AutoRetransmitCount = 3,        // 最大重传次数 3
        .AutoRetransmitDelay = 5,        // 最大重传延迟 1250us
    }

};

RF_HandleTypeDef hrf;
void rf_simple_init(void)
{
    // Rf_Init();           // 射频模块初始化
    // SetDataRate(2);      // 设置速率为2Mbps

    // uart_printf("in 2.4 mode1===============\r\n");
    // xvr_reg_initial_24();
    // HAL_RF_StructInit(&hrf.Params);
    // HAL_RF_Init(&hrf,&hrf.Params);

    HAL_RF_Init(&hrf,&Init_S_normal);
}

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


void bk24_send_data(void)
{
    static uint32_t packet_count = 0;
    uint8_t rf_fifo_data[32];

    for(uint8_t i = 0; i < 32; i++) 
    {
        rf_fifo_data[i] = i;
    }

    __HAL_RF_Set_TxMode();

    while(1)
    {
        __HAL_RF_CMD_FLUSH_TXFIFO();      // 清空TX FIFO
        __HAL_RF_CLEAR_IRQ_FLAGS(RX_DR|TX_DS|MAX_RT|RX_P_NO);  // 清除中断标志
        
        // 写入数据并发送
        RF_Write_fifo(rf_fifo_data, 32);
        __HAL_RF_CHIP_EN();
        Delay_us(15);
        __HAL_RF_CHIP_DIS();
        
        packet_count++;
        
        // 简单打印，每100包打印一次
        if(packet_count % 100 == 0) 
        {
            uart_printf("send %ld packet\n", packet_count);
        }
        
        // 简单延时，控制发送速率
        Delay_us(100);
    }
}

void rf_simple_receive()
{
    __HAL_RF_Set_RxMode();
    uint8_t rec[32];
    uint16_t count_packet; 
    uint16_t count_printf; 
    while(1)
    {
        uint8_t status = __HAL_RF_GET_IRQ_Status();
        if (__HAL_RF_GET_IRQ_FLAGS(RX_DR) == SET)
        {
            RF_Read_fifo(rec,32);
            __HAL_RF_CLEAR_IRQ_FLAGS(RX_DR);
            count_packet++;
            uart_printf("recieve %ld packet: ", count_packet);
            // 按照十进制打印接收到的数据
            for(uint8_t i = 0; i < 32; i++)
            {
                uart_printf("%d ", rec[i]);
            }
            uart_printf("\n");
        }
        else
        {
            count_printf++;
            if(count_printf % 10000 == 0)
            {
                uart_printf("not received\n");
            }
        }
    }
}