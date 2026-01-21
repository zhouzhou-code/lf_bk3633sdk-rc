#include "rf_handler.h"
#include "hal_drv_rf.h"
#include "user_config.h"
#include "my_queue.h"
#include "Types.h"


//简化处理，定义固定长度发送和接收队列，无线端每次接收定长32字节
uint8_t rf_send_queue_buffer[MAX_RF_PAYLOAD_LEN*rf_send_queue_len]; //发送队列缓冲区
my_queue_t rf_txQueue;               // 发送队列
uint8_t rf_recv_queue_buffer[MAX_RF_PAYLOAD_LEN*rf_recv_queue_len]; //接收队列缓冲区
my_queue_t rf_rxQueue;               // 接收队列




RF_HandleTypeDef hrf; //全局RF句柄
uint16_t isq_count_rxdr = 0;
uint16_t isq_count_txds = 0;
uint16_t isq_count_maxrt = 0;
//接收中断回调函数,数据push到接收队列
void rxdr_callback(RF_HandleTypeDef *hrf)
{
    if(hrf->RxBuff_valid == 1) {
        hrf->RxBuff_valid = 0;
        queue_push_overwrite(&rf_rxQueue, hrf->RxBuff);
    }
    isq_count_rxdr++;
}
void txds_callback(RF_HandleTypeDef *hrf)
{
    isq_count_txds++;
    uart_printf("tx=%d\n", isq_count_txds);   
}
void maxrt_callback(RF_HandleTypeDef *hrf)
{
    isq_count_maxrt++;
    uart_printf("rt=%d\n", isq_count_maxrt);
}

//初始化参数配置结构体
RF_ConfgTypeDef Init_S=
{
    .Mode = MODE_TX,
    .DataRate = BPS_2M,
    .TxPower = RF_TX_POWER_0dBm,
    .Channel = 0x05, // 频道5
    .Protocol ={
        .AddressWidth = 5,
        .TxAddress = {0x10, 0x11, 0x36, 0x00, 0x00},
        .Support_NoAck = 1,             
        .AutoRetransmitDelay = 2,        // 最大重传延迟 750us
        .AutoRetransmitCount = 3,        // 最大重传次数 15
        
        .RxPipes[0] = {
            .PipeNum = 0,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x10, 0x11, 0x36, 0x00, 0x00}
        },
        .RxPipes[1] = {
            .PipeNum = 1,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x10, 0x56, 0x24, 0xCD, 0x78}
        },
        .RxPipes[2]={
            .PipeNum = 2,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x11, 0, 0, 0, 0}
        },
        .RxPipes[3]={
            .PipeNum = 3,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x12, 0, 0, 0, 0}
        },
        .RxPipes[4]={
            .PipeNum = 4,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x13, 0, 0, 0, 0}
        },
        .RxPipes[5]={
            .PipeNum = 5,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x14, 0, 0, 0, 0}
        }
    },
    .IRQ = {
        .RxDR = {
            .enable = 1,
            .user_cb = rxdr_callback
        },
        .TxDS = {
            .enable = 1,
            .user_cb = txds_callback
        },
        .MaxRT = {
            .enable = 1,
            .user_cb = maxrt_callback
        }
    }
};


void RF_Handler_Init(void)
{
    /* 初始化RF模块 */
    HAL_RF_Init(&hrf, &Init_S);
    HAL_RF_SetTxMode(&hrf);
    /* 初始化发送和接收队列 */
    queue_init(&rf_txQueue, rf_send_queue_buffer, 
                MAX_RF_PAYLOAD_LEN*rf_send_queue_len, rf_send_queue_len, MAX_RF_PAYLOAD_LEN);
    queue_init(&rf_rxQueue, rf_recv_queue_buffer,
                MAX_RF_PAYLOAD_LEN*rf_recv_queue_len, rf_recv_queue_len, MAX_RF_PAYLOAD_LEN);

    
}

void RF_txQueue_Send(uint8_t *data_pack, uint8_t len)
{
    if(len > MAX_RF_PAYLOAD_LEN) {
        len = MAX_RF_PAYLOAD_LEN; // 限制最大长度
    }
    // 入队
    queue_push_overwrite(&rf_txQueue, data_pack);
}
uint8_t RF_rxQueue_Recv(uint8_t *data_pack, uint8_t len)
{
    if(len > MAX_RF_PAYLOAD_LEN) {
        len = MAX_RF_PAYLOAD_LEN; // 限制最大长度
    }
    // 出队
    return queue_pop(&rf_rxQueue, data_pack);
}


// RF处理主函数，定时运行
void RF_Send_Handler(RF_HandleTypeDef *hrf)
{
    // 处理发送队列
    uint8_t tx_data[MAX_RF_PAYLOAD_LEN];
    // 尝试从发送队列取数据
    if(queue_pop(&rf_txQueue, tx_data) == 1) {
        // 有数据，发送
        if(hrf->Cur_Mode != MODE_TX) {
            HAL_RF_SetTxMode(hrf);
        }
        HAL_RF_Transmit_IT(hrf, tx_data, MAX_RF_PAYLOAD_LEN);
    }
    
}
