#include <stdio.h>
#include "app.h"

#include "driver_rf.h"
#include "driver_bk24.h"
#include "gpio.h"
#include "driver_timer.h"
#include "application_mode.h"
#include "bk3633_reglist.h"

#define UART_PRINTF    uart_printf
int uart_printf(const char *fmt,...);

extern SYSTEM_STRUCT_DATA system_data;
extern uint32_t RF_flag;
extern volatile uint8_t uRF_TXTime;

uint32 Cfg_0c_Val[4] = {0x1F, 0x1B, 0x73, 0x00};
uint32 Cfg_0d_Val[4] = {0x36, 0xB4, 0x80, 0x28};
uint32 P0_Address[5] = {0x10, 0x11, 0x36, 0x0, 0x0};   // {0x03, 0x01, 0xe7, 0xa5, 0xbc};
uint32 rf_address_tx_pub[5] = {0x50,0x25, 0x34, 0x18, 0x24};    // {0x50,0x00, 0x00, 0x00, 0x00};

uint32 P1_Address[5] = {0x10, 0x56, 0x24, 0xCD, 0x78};
uint8_t rf_fifo_data[MAX_PACKET_LEN];             //rf fifo
//uint8 RF_CHANNEL_TABLE[16] = {8, 37, 68, 21, 40, 77, 7, 35, 67, 10, 42, 55, 14, 28, 49, 41};//{3, 28, 53, 8, 33, 58, 13, 38, 63, 18, 43, 68, 23, 48, 73, 78};
uint8_t RF_CHANNEL_TABLE[4][4] = {{7, 8, 10, 14}, {21, 28, 35, 37}, {40, 41, 42, 49}, {55, 67, 68, 77}};    // { {8,37,68,21},{40, 77, 7, 35},{67, 10, 42, 55},{14, 28, 49, 41} };

uint8_t uRXDataLens;
uint8_t flag_test;
uint8_t channel_search;

/*! \fn void driver_rf_spi_write_register(UINT8 reg,  UINT8 value)
    \brief Write the value of register address \reg.
    \param reg - register address
    \param value - data
    \return void
*/
void PowerUp_Rf(void) 
{
    TRX_CONFIG |= 0x02;
}
void PowerDown_Rf(void)
{
    TRX_CONFIG &= 0xFD;
}
void SwitchToRxMode(void)
{
    PowerUp_Rf();
    FLUSH_RX;
    TRX_CE = 0;
    TRX_CONFIG |= 0x01;
    TRX_CE = 1;
}

void SwitchToTxMode(void)
{
    PowerUp_Rf();
    FLUSH_TX;
    TRX_CE = 0;
    TRX_CONFIG &= 0xFE;
    TRX_CE = 1;
}
void ChangeTxPipe(uint8_t pipe)
{
    uint32 tmp_address[5];
    switch(pipe)
    {
    case 0:
        memcpy_2461(&TRX_RX_ADDR_P0_0, P0_Address, RF_ADDRESS_LEN);
        memcpy_2461(&TRX_TX_ADDR_0, P0_Address, RF_ADDRESS_LEN);
        memcpy_2461(&TRX_RX_ADDR_P1_0, P1_Address, RF_ADDRESS_LEN);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        memcpy_2461(tmp_address, P1_Address, RF_ADDRESS_LEN);
        if(pipe == 2)                              // For P1, need not change LSB
        {
            tmp_address[0] = TRX_RX_ADDR_P2;
        }
        else if(pipe == 3)
        {
            tmp_address[0] = TRX_RX_ADDR_P3;
        }
        else if(pipe == 4)
        {
            tmp_address[0] = TRX_RX_ADDR_P4;
        }
        else if(pipe == 5)
        {
            tmp_address[0] = TRX_RX_ADDR_P5;
        }
        memcpy_2461(&TRX_RX_ADDR_P0_0, tmp_address, RF_ADDRESS_LEN);
        memcpy_2461(&TRX_TX_ADDR_0, tmp_address, RF_ADDRESS_LEN);
        memcpy_2461(tmp_address, P1_Address, RF_ADDRESS_LEN);
        tmp_address[2] = 0x37;                          // avoid for 2 same address for R1, R2, ... R5
        memcpy_2461(&TRX_RX_ADDR_P1_0, tmp_address, RF_ADDRESS_LEN);
        break;
    default:
        break;
    }
}
void R_RX_PAYLOAD(uint8_t *pBuf, uint8_t bytes)
{
    uint8_t i;
    TRX_CMD = 0x40;                   // command
    for(i=0; i<bytes; i++)
    {
        pBuf[i] = TRX_FIFO;    // data
    }
    TRX_CMD = 0x00;                   // end: nop command
}
void W_TX_PAYLOAD(uint8_t *pBuf, uint8_t bytes)
{
    uint8_t i;
    TRX_CMD = 0x60;
    for(i=0; i<bytes; i++)
    {
        TRX_FIFO = pBuf[i];
    }
    TRX_CMD = 0x00;
}
void W_TX_PAYLOAD_NOACK(uint8_t *pBuf, uint8_t bytes)
{
    uint8_t i;
    TRX_CMD = 0x68;//0xB0;
    for(i=0; i<bytes; i++)
    {
        TRX_FIFO = pBuf[i];
    }
    TRX_CMD = 0x00;
}
void W_ACK_PAYLOAD(uint8_t *pBuf, uint8_t bytes, uint8_t pipe)//预制应答数据
{
    uint8_t i;
    TRX_CMD = 0x68 + pipe;
    for(i=0; i<bytes; i++)
    {
        TRX_FIFO = pBuf[i];
    }
    TRX_CMD = 0x00;
}
void Rf_Init(void)
{
    clrf_SYS_Reg0x3_bk24_pwd;
  //   set_SYS_Reg0x2_core_div(1);
    //  set_SYS_Reg0x2_core_sel(3);

//    set_SYS_Reg0x1e_ahb_clk_gate_disable(9);
//    uint8 i;
//    // 锟斤拷始锟斤拷模锟斤拷拇锟斤拷锟�
//    for(i=0; i<8; i++)
//        Write_Reg_Ana(i, Ana_Init_Val[i]);
//    // 锟斤拷锟矫癸拷锟斤拷
//    TRX_GPA0 = 0x05;
//    TRX_GPA1 = 0xF8;
//    // 锟截憋拷模式选锟斤拷锟铰�
//    TRX_ANAPWD_CTRL0 |= 0x11;
    // 锟斤拷始锟斤拷cfg_0C, cfg_0D
    memcpy_2461(&TRX_CFG_0C_0, Cfg_0c_Val, 4);
    memcpy_2461(&TRX_CFG_0D_0, Cfg_0d_Val, 4);
//    memcpy_2461(&TRX_RAMP_TABLE_0, Ramp_Table_Val, 11);
    TRX_CONFIG     = 0x0C;     // default PTX
    TRX_EN_AA      = 0x3F;     // 使能6个管道自动应答
    TRX_EN_RXADDR  = 0X3F;     // 使能6个管道接收地址
    TRX_SETUP_AW   = 0x03;      // 地址宽度5字节
    TRX_SETUP_RETR = 0x33;      // 自动重传延时750us, 重传3次
    TRX_RF_CH      = 0x05;       // 频道5
    TRX_RF_SETUP   = 0x0f;      // 2Mbps
    
    //管道0~5接收地址 pipeline address
    memcpy_2461(&TRX_RX_ADDR_P0_0, P0_Address, RF_ADDRESS_LEN); 
    memcpy_2461(&TRX_RX_ADDR_P1_0, P1_Address, RF_ADDRESS_LEN);
    TRX_RX_ADDR_P2 = 0x11;
    TRX_RX_ADDR_P3 = 0x12;
    TRX_RX_ADDR_P4 = 0x13;
    TRX_RX_ADDR_P5 = 0x14;

    //发送地址 pipeline address ;发送地址应该和目标设备的接收地址相同，
    //发送设备自动把发送地址合并到数据包中并广播，
    //接收设备收到数据后，将会比对数据包地址和本设备接收地址是否一致
    memcpy_2461(&TRX_TX_ADDR_0, P0_Address, RF_ADDRESS_LEN);
    
    //pipline 0~5 单次数据包长度
    TRX_RX_PW_P0 = 0x20; //32字节
    TRX_RX_PW_P1 = 0x20;
    TRX_RX_PW_P2 = 0x20;
    TRX_RX_PW_P3 = 0x20;
    TRX_RX_PW_P4 = 0x20;
    TRX_RX_PW_P5 = 0x20;

    TRX_DYNPD    = 0x3F; //使能6个pipeline动态有效载荷长度
    TRX_FEATURE  = 0x07; //使能动态有效载荷,ACK包中附加有效载荷,动态ACK
//    TRX_PLL_SDM &= 0xEF;    // open_loop_en=0
    PowerDown_Rf();
    
    //设置xvr寄存器初始值
    addXVR_Reg0x24 = 0x000e0782;
    addXVR_Reg0x3b = 0x36340048;

}
void memcpy_2461(volatile uint32 *p, uint32 *val, uint8 len)
{
    uint8_t i = 0;
    for ( i = 0; i < len ; i++ )
    {
        p[i] = val[i];
    }
}


/*
*设置数据传输速率，同时配置rf寄存器和xvr无线收发寄存器的相关参数
* data_rate: 0--250kbps; 1--1Mbps; 2--2Mbps
*/
void SetDataRate(int data_rate)
{
    switch ( data_rate )
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
            break;
    }
}

//干扰时使用这个函数 跳频
void driver_rf_channel_switch(void)
{
    uint8_t uTmp;
    #if    0
    system_data.rf_channel++;
    if(system_data.rf_channel > 3)
        system_data.rf_channel = 0;
    #endif

    system_data.rf_array++;
    if(system_data.rf_array > 3)
        system_data.rf_array = 0;
    uTmp = system_data.rf_channel;
    if(system_data.rf_array == 0x1)
        uTmp = uTmp >> 2;
    else if(system_data.rf_array == 0x02)
        uTmp = uTmp >> 4;
    else if(system_data.rf_array == 0x03)
        uTmp = uTmp >> 6;
    uTmp &= 0x03;

    if(!flag_test)
    {
        TRX_RF_CH = 0x80+RF_CHANNEL_TABLE[system_data.rf_array][uTmp];
        TRX_CE = 0;
        TRX_CE = 1;
    }
}

//设置通信频道
void driver_set_rf_channel(uint8_t uArray, uint8_t uIndx)
{
    TRX_CE = 0;
    TRX_RF_CH = 0x80+RF_CHANNEL_TABLE[uArray][uIndx];
    TRX_CE = 1;
}

// uint32_t driver_rf_data_send(uint8_t len)
// {
//     uint8_t status;
//     uint8_t fifo_status_rx;
//     uint8_t rx_len;
//     uint8_t bTXOvTime;

//     RF_CMD_FLUSH_TX;
//     RF_CMD_FLUSH_RX;
//     RF_CMD_CLEAR_STATUS_ALL;
//     RF_flag &= ~flag_rf_last_state;
//     W_TX_PAYLOAD(rf_fifo_data,len);

//     bTXOvTime = 0x0;
//     uRF_TXTime = 0x0;
//     do
//     {
//         status=TRX_IRQ_STATUS;
//         Delay_us(5);
//         if(uRF_TXTime > 5)
//         {
//             bTXOvTime = 1;
//             break;
//         }
//     }
//     while((status & (STATUS_TX | STATUS_RX_DR)) == 0x00);

//     if(status & STATUS_TX_DS)
//     {

//         RF_flag |= flag_rf_last_state ;
//         RF_flag |= flag_rf_send_success;
//         if(system_data.rf_connect != SYSTEM_NEXT_CH)
//         {
//             system_data.rf_send_length = 0;
//             system_data.wheel = 0;
//             #if MOUSE_SWING
//             system_data.swing = 0x0;
//             #endif
//         }
//         //  driver_rf_receive_MSG_package();
//         if(system_data.rf_connect == SYSTEM_SYNCHRONIZE)
//         {
//             driver_timer0_reset();
//         }
//         else if(TRX_OBSERVE_TX & 0x0F)
//         {
//             if(system_data.rf_connect != SYSTEM_NEXT_CH)
//             {
//  //               driver_timer0_reset();
//             }
//         }
//         RF_CMD_FLUSH_TX;
//         RF_CMD_CLEAR_STATUS_TX;
//     }
//     else
//     {

//     }

//     if(status & STATUS_RX_DR)
//     {

//         do
//         {
//             rx_len = TRX_RX_RPL_WIDTH;
//             if(rx_len <= MAX_PACKET_LEN)
//             {
//                 //memset(rf_fifo_data, 0, 32);
//                 R_RX_PAYLOAD(rf_fifo_data,len);            // read receive payload from RX_FIFO buffer
//                 if((rf_fifo_data[0] == DATATYPE_MOUSE) && (rf_fifo_data[1] == DATATYPE_MOUSE_SLEEP))
//                 {
//                     RF_flag |= flag_system_powerdown;
//                 }
//             }

//             fifo_status_rx = TRX_IRQ_STATUS;                // read register FIFO_STATUS's value
//         }while((fifo_status_rx & STATUS_RX_EMPTY) == 0);    // while not empty

//         uart_printf("rx\n");
//         RF_CMD_FLUSH_RX;
//         RF_CMD_CLEAR_STATUS_RX;
//     }

//     // 发送超时处理
//     if(bTXOvTime)
//     {
//         RF_CMD_FLUSH_RX;
//         RF_CMD_FLUSH_TX;
//         PowerDown_Rf();
//         PowerUp_Rf();
//         RF_CMD_CLEAR_STATUS_MAX;
//     }
// //锟斤拷为max锟斤拷锟斤拷要锟截达拷锟斤拷只要锟斤拷max锟酵匡拷锟斤拷锟斤拷锟斤拷
// //rf_send_exit:
//     //if(status & STATUS_MAX_RT)
//     //{
//     //RF_CMD_FLUSH_TX;
//     //RF_CMD_FLUSH_RX;
//     //RF_CMD_CLEAR_STATUS_ALL;
//     //}
//     return (RF_flag&flag_rf_last_state);
// }

/**
 * @brief RF数据发送函数
 * @param len 要发送的数据长度（字节）
 * @return 发送状态标志（RF_flag & flag_rf_last_state）
 * 
 * 1. 清空TX/RX FIFO和状态寄存器
 * 2. 将数据写入TX FIFO并启动发送
 * 3. 等待发送完成（或超时）
 * 4. 处理发送成功、接收数据、超时等不同情况
 * 5. 返回最终的发送状态

 */
uint32_t driver_rf_data_send(uint8_t len)
{
    uint8_t status;           // RF中断状态寄存器值
    uint8_t fifo_status_rx;   // RX FIFO状态
    uint8_t rx_len;           // 接收到的数据长度
    uint8_t bTXOvTime;        // 发送超时标志

    // 1.1 清空发送和接收FIFO
    RF_CMD_FLUSH_TX;     // 清空TX FIFO，准备发送新数据
    RF_CMD_FLUSH_RX;     // 清空RX FIFO，避免旧数据干扰
    
    // 1.2 清除所有中断状态标志
    RF_CMD_CLEAR_STATUS_ALL;
    // 1.3 清除上次发送状态标志
    RF_flag &= ~flag_rf_last_state;
    
    // 1.4 将待发送数据写入TX FIFO
    // W_TX_PAYLOAD宏相当于执行：TRX_CMD = 0x60（写TX有效载荷命令）
    // 然后将rf_fifo_data中的len字节数据写入TRX_FIFO寄存器
    W_TX_PAYLOAD(rf_fifo_data, len);
    
    bTXOvTime = 0x0;      // 初始化超时标志为0（未超时）
    uRF_TXTime = 0x0;     // 发送时间计数器清零
    
    // 循环等待发送完成或超时
    do
    {
        status = TRX_IRQ_STATUS;  // 读取中断状态寄存器
        Delay_us(5);              
        
        // 检查是否超时（等待超过5次循环，即25μs）
        if(uRF_TXTime > 5) //uRF_TXTime在定时器中断中更新
        {
            bTXOvTime = 1;  // 设置超时标志
            break;          // 退出等待循环
        }
    }
    // 等待条件：既没有发送完成中断，也没有接收到数据中断
    while((status & (STATUS_TX | STATUS_RX_DR)) == 0x00);
    
    /* 处理发送成功情况 */
    if(status & STATUS_TX_DS)  // 发送成功，收到ACK确认
    {
        // 3.1 设置发送成功标志
        RF_flag |= flag_rf_last_state;      // 设置上次发送状态为成功
        RF_flag |= flag_rf_send_success;    // 设置发送成功标志
        
        // 3.2 如果不是频道切换模式，重置发送相关状态
        if(system_data.rf_connect != SYSTEM_NEXT_CH)
        {
            system_data.rf_send_length = 0;  // 发送长度清零
            system_data.wheel = 0;           // 滚轮数据清零
            
            #if MOUSE_SWING  // 如果启用了鼠标摆动功能
            system_data.swing = 0x0;         // 摆动数据清零
            #endif
        }
        
        if(system_data.rf_connect == SYSTEM_SYNCHRONIZE)
        {
            // 同步模式下重置定时器0
            driver_timer0_reset();
        }
        else if(TRX_OBSERVE_TX & 0x0F)  // 检查重传次数是否不为0
        {
            // 如果不是频道切换模式且发生了重传
            if(system_data.rf_connect != SYSTEM_NEXT_CH)
            {
                // driver_timer0_reset();
            }
        }
        
        // 3.4 清理发送缓冲区
        RF_CMD_FLUSH_TX;         // 清空TX FIFO
        RF_CMD_CLEAR_STATUS_TX;  // 清除发送完成中断标志
    }
    else
    {
        
    }
    
    if(status & STATUS_RX_DR)  // 如果在发送过程中收到了数据（可能是ACK或其他）
    {
        // 4.1 循环读取RX FIFO中的所有数据
        do
        {
            // 获取接收到的数据长度
            rx_len = TRX_RX_RPL_WIDTH;
            
            // 4.2 检查数据长度是否有效
            if(rx_len <= MAX_PACKET_LEN)  // 长度在合理范围内
            {
                // 从RX FIFO读取接收到的数据
                // 注意：这里使用了传入的len参数，但应该使用rx_len
                // 可能存在缓冲区溢出风险
                R_RX_PAYLOAD(rf_fifo_data, len);
                
                // 4.3 检查是否是鼠标休眠命令
                if((rf_fifo_data[0] == DATATYPE_MOUSE) && 
                   (rf_fifo_data[1] == DATATYPE_MOUSE_SLEEP))
                {
                    RF_flag |= flag_system_powerdown;  // 设置系统休眠标志
                }
            }
            
            // 4.4 再次读取FIFO状态，检查是否还有数据
            fifo_status_rx = TRX_IRQ_STATUS;
            
        } while((fifo_status_rx & STATUS_RX_EMPTY) == 0);  // 循环直到RX FIFO为空
        
        // 调试输出：表示收到了数据
        uart_printf("rx\n");
        
        // 4.5 清理接收缓冲区
        RF_CMD_FLUSH_RX;          // 清空RX FIFO
        RF_CMD_CLEAR_STATUS_RX;   // 清除接收数据就绪中断标志
    }
    
    /*  */
    if(bTXOvTime)  // 如果发送超时
    {
        // 5.1 清理所有FIFO
        RF_CMD_FLUSH_RX;
        RF_CMD_FLUSH_TX;
        
        // 5.2 硬件复位RF模块
        PowerDown_Rf();  // 关闭RF模块电源
        PowerUp_Rf();    // 重新上电RF模块
        
        // 5.3 清除最大重试次数中断标志
        RF_CMD_CLEAR_STATUS_MAX;
    }
    
   //锟斤拷为max锟斤拷锟斤拷要锟截达拷锟斤拷只要锟斤拷max锟酵匡拷锟斤拷锟斤拷锟斤拷
//rf_send_exit:
    //if(status & STATUS_MAX_RT)
    //{
    //RF_CMD_FLUSH_TX;
    //RF_CMD_FLUSH_RX;
    //RF_CMD_CLEAR_STATUS_ALL;
    //}

    // 返回发送状态标志（主要检查flag_rf_last_state）
    return (RF_flag & flag_rf_last_state);
}
uint32 driver_rf_data_retrans(void)
{
    uint8_t status;
    uint8_t bTXOvTime;

    RF_CMD_CLEAR_STATUS_MAX;
//    delay_us(20);
    bTXOvTime = 0x0;
    uRF_TXTime = 0x0;
    do
    {
        status = TRX_IRQ_STATUS;
        Delay_us(5);
        if(uRF_TXTime > 5)
        {
            bTXOvTime = 1;
            break;
        }
        #if    0
        if(RF_flag & flag_tick_8ms_loop)
        {
            break;
        }

        if(!gpio_get_input(KEY_PAIR))
        {
            app_set_device_index(DEVICE_OTHER);
            break;
        }
        #endif
    }
    while((status & STATUS_TX) == 0x00);

    if(status & STATUS_TX_DS)
    {
        RF_flag |= flag_rf_last_state ;
        if(system_data.rf_connect != SYSTEM_NEXT_CH)
        {
            system_data.rf_send_length = 0;
            system_data.wheel = 0;
            #if MOUSE_SWING
            system_data.swing = 0x0;
            #endif
        }
        if((system_data.rf_connect == SYSTEM_SYNCHRONIZE))
        {
            driver_timer0_reset();
        }
        else if(TRX_OBSERVE_TX&0x0F)
        {
            if(system_data.rf_connect != SYSTEM_NEXT_CH)
            {
//                driver_timer0_reset();
            }
//            uart_printf("re=%d\n",TRX_OBSERVE_TX&0x0F);
        }
        RF_CMD_FLUSH_TX;
        RF_CMD_CLEAR_STATUS_TX;
    }
    if(status & STATUS_MAX_RT)
    {
        RF_flag &= ~flag_rf_last_state ;
    }

    if(bTXOvTime)
    {
        RF_CMD_FLUSH_RX;
        RF_CMD_FLUSH_TX;
        PowerDown_Rf();
        PowerUp_Rf();
        RF_CMD_CLEAR_STATUS_MAX;

        RF_flag &= ~flag_rf_last_state ;
        RF_flag &= ~flag_rf_hop_16ch;            // 锟斤拷锟斤拷锟斤拷fifo, 锟斤拷锟斤拷锟絝ifo锟斤拷锟斤拷driver_rf_data_retrans锟斤拷锟斤拷
    }
    return (RF_flag & flag_rf_last_state);
}
void driver_rf_maxRT_clear(void)
{
    //RF_CMD_FLUSH_TX;
    //RF_CMD_FLUSH_RX;
    //RF_CMD_CLEAR_STATUS_ALL;
    RF_CMD_FLUSH_TX;
    RF_CMD_FLUSH_RX;
    RF_CMD_CLEAR_STATUS_ALL;
}

uint8_t driver_rf_data_send_noACK(uint8_t len)
{
    uint8_t status;
    RF_CMD_FLUSH_RX;
    RF_CMD_FLUSH_TX;
    RF_CHIP_ENABLE;
    W_TX_PAYLOAD_NOACK(rf_fifo_data,len);
    //sleep and wake up by rf interrupt
    //ENABLE_RF_WAKEUP;
    //driver_idle_osc8mhz();
    //DISABLE_RF_WAKEUP;
    do
    {
        status = TRX_IRQ_STATUS;
    }
    while((status & STATUS_TX) == 0x00);
    
    if(status & STATUS_TX_DS)
    {
        //RF_CMD_FLUSH_TX;
        RF_CMD_CLEAR_STATUS_TX;
    }
    else
    {
    }
    RF_CHIP_DISABLE;
    return 1;
}

uint8_t driver_rf_data_send_ACK(uint8_t len)
{
    uint8_t status, uRes = 0x0;
    uint8_t bTXOvTime;
    RF_CMD_FLUSH_RX;
    RF_CMD_FLUSH_TX;
    RF_CMD_CLEAR_STATUS_ALL;
    RF_CHIP_ENABLE;
    W_TX_PAYLOAD(rf_fifo_data,len);
    //sleep and wake up by rf interrupt
    //ENABLE_RF_WAKEUP;
    //driver_idle_osc8mhz();
    //DISABLE_RF_WAKEUP;
    bTXOvTime = 0x0;
    uRF_TXTime = 0x0;
    do
    {
        status = TRX_IRQ_STATUS;
        if(uRF_TXTime > 5)
        {
            bTXOvTime = 1;
            break;
        }
    }
    while((status & STATUS_TX) == 0x00);

    if(status & STATUS_RX_DR)
    {
        RF_CMD_FLUSH_RX;
        RF_CMD_CLEAR_STATUS_RX;
        uRes = 2;
    }

    if(status & STATUS_TX_DS)
    {
        RF_CMD_FLUSH_TX;
        RF_CMD_CLEAR_STATUS_TX;
        uRes = 1;
    }

    if((status & STATUS_MAX_RT) || bTXOvTime)
    {
        RF_CMD_FLUSH_RX;
        RF_CMD_FLUSH_TX;
        PowerDown_Rf();
        PowerUp_Rf();
        RF_CMD_CLEAR_STATUS_MAX;
    }

    RF_CHIP_DISABLE;
    return uRes;
}

/*! \fn void driver_rf_receive_package(void)
    \brief Receive the RF package to fifo buffer.
    \param void
    \return void
*/
void driver_rf_receive_package(void)
{
    uint8_t status;
    uint8_t fifo_status;
    uint8_t len;
    status=TRX_IRQ_STATUS;

//   uart_printf("s=%x\r\n",status);
// read register STATUS's value
    if(status & STATUS_RX_DR)            // if receive data ready (RX_DR) interrupt
    {
        do
        {
            len = TRX_RX_RPL_WIDTH;
            if(len <= MAX_PACKET_LEN)
            {
                R_RX_PAYLOAD(rf_fifo_data,len);            // read receive payload from RX_FIFO buffer
                
                if(!(RF_flag&flag_rf_receive_page) && 
                ((rf_fifo_data[0] == DATATYPE_PAGE_START) || 
                (rf_fifo_data[0] == DATATYPE_PAGE_END_MOUSE)))
                {
//                    if(rf_fifo_data[3] & 0x01)
                    {
                        RF_flag |= flag_rf_receive_page;
                        uRXDataLens = len;
                    }
                }
            }
            else
            {
                RF_CMD_FLUSH_RX;
                break;
            }
            fifo_status = TRX_IRQ_STATUS;               // read register FIFO_STATUS's value
        }
        while((fifo_status & STATUS_RX_EMPTY) != STATUS_RX_EMPTY);  // check the rx fifo is empty         // while not empty
      TRX_IRQ_STATUS = fifo_status;
      }

}
extern uint32_t XVR_REG24_BAK;
extern volatile uint32_t XVR_ANALOG_REG_BAK[32];

void driver_rf_ouput_power_value_set(uint8_t power_level)
{
    uint32_t regv_0x24;
    XVR_ANALOG_REG_BAK[4] |=1<<29;
    addXVR_Reg0x4 = XVR_ANALOG_REG_BAK[4];
    regv_0x24=addXVR_Reg0x24&0xFFFFF87F;
    /*    switch(power_level)
        {
          case 0x0:
            power_level=0;
            break;
          case 0x1:
            power_level=1;
            break;
          default:
            power_level=6;
            break;
        }*/
    regv_0x24=regv_0x24|(power_level<<7);
    addXVR_Reg0x24 = regv_0x24;
    XVR_REG24_BAK = addXVR_Reg0x24;

 //   XVR_ANALOG_REG_BAK[4] &= ~1<<29;
 //   addXVR_Reg0x4 = XVR_ANALOG_REG_BAK[4];

//  uart_printf("24= %2x\r\n",XVR_REG24);
}

/*
关闭AGC（自动增益控制）：
在读取RSSI前，需要关闭AGC（通过 addXVR_Reg0x3c |= 0x01<<31;），以保证RSSI值的准确性。

设置RX增益：
需要将射频接收增益（rx gain）设置为固定值（通过操作 XVR_ANALOG_REG_BAK[8] 和 addXVR_Reg0x8），
避免增益变化影响RSSI读数。
*/
uint8_t get_rf_rssi(void)
{
    uint8_t rssi_data = 0;
    rssi_data = get_XVR_Reg0x13_rssi_ind_o;
    return(rssi_data);
}

