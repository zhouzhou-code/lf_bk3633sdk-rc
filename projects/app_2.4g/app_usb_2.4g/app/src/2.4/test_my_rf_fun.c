#include "app.h"
#include "bk3633_reglist.h"
#include "icu.h"
#include "driver_timer.h"
#include "application_mode.h"
#include "application_mode_test.h"
#include "application_mode_PCBA.h"
#include <string.h>
#include "driver_sensor.h"

#include "gpio.h"

#include <string.h>

extern uint32 P0_Address[5] ;
extern uint32_t RF_flag;
extern uint8_t local_addr[6];
extern APP_VAL_T     app_val;
extern void  xvr_reg_initial_24(void);

#define UART_PRINTF    uart_printf
int uart_printf(const char *fmt,...);

// void f24Init(void)
// {
//    //f24_xvr_init();
//     Rf_Init();
//     SetDataRate(2);  // data rate:2Mbp
// }


// void fWorkModeInit(void)
// {

//     channel_search = 0;
//     flag_test = 0;
//     RF_flag = 0x0;

//     RF_POWER_UP;
//     RF_CMD_FLUSH_TX;
//     RF_CMD_FLUSH_RX;
//     SwitchToTxMode();
//     if(app_bt.bPwrOnModeCK)
//     {
//         app_bt.bPwrOnModeCK = 0x0;
//         if((!gpio_get_input(KEY_LEFT)) && (!gpio_get_input(KEY_RIGHT)) && (!gpio_get_input(KEY_MIDDLE)))
//         {
//             system_data.system_mode = SYSTEM_TEST;
//         }
//         else if((!gpio_get_input(KEY_RIGHT)) && (!gpio_get_input(KEY_MIDDLE)))
//         {
//             system_data.system_mode = SYSTEM_PCBA;
//         }
//         else
//         {
//             system_data.system_mode = SYSTEM_NORMAL;
//         }
//     }
//     else
//     {
//         system_data.system_mode = SYSTEM_NORMAL;
//     }

//     if(system_data.system_mode == SYSTEM_NORMAL)
//     {
//         driver_rf_ouput_power_value_set(cRFPWR);
//         general_dongle_search();
//     }
// }
// void fn24main(void)
// {
//     uart_printf("in 2.4 mode1===============\r\n");
//     xvr_reg_initial_24();

//     application_initial();

//     app_set_24page_timer();

//     fWorkModeInit();
// }


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