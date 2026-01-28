#include "rf_handler.h"
#include "hal_drv_rf.h"
#include "user_config.h"
#include "my_queue.h"
#include "Types.h"
#include "timer_handler.h"


/* rf_txQueue每一项是固定长度为32+1字节，第一字节存放净荷长度，发送时只发送净荷 */

uint8_t rf_send_queue_buffer[txqueue_item_size*rf_send_queue_len]; //发送队列缓冲区
my_queue_t rf_txQueue;               // 发送队列
uint8_t rf_recv_queue_buffer[rxqueue_item_size*rf_recv_queue_len]; //接收队列缓冲区
my_queue_t rf_rxQueue;               // 接收队列


RF_HandleTypeDef hrf; //全局RF句柄
//中断计数
uint32_t rf_int_count_rxdr = 0;
uint32_t rf_int_count_txds = 0;
uint32_t rf_int_count_maxrt = 0;


//接收中断回调函数,数据push到接收队列
void rxdr_callback(RF_HandleTypeDef *hrf)
{
    if(hrf->RxBuff_valid == 1) {
        hrf->RxBuff_valid = 0;
        uint8_t temp[1+1+hrf->RxLen];
        temp[0] = hrf->RxLen;               // 第一字节为有效长度
        temp[1] = hrf->RxPipes;             // 第二字节为接收管道          
        memcpy(&temp[2], hrf->RxBuff, hrf->RxLen);
        
        queue_push_overwrite(&rf_rxQueue, temp);
    }
    rf_int_count_rxdr++;
}
void txds_callback(RF_HandleTypeDef *hrf)
{
    rf_int_count_txds++;
    uart_printf("tx=%d\n", rf_int_count_txds);   
}
void maxrt_callback(RF_HandleTypeDef *hrf)
{
    rf_int_count_maxrt++;
    uart_printf("rt=%d\n", rf_int_count_maxrt);
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
    HAL_RF_TimeManager_register(&hrf, Get_SysTick_ms);

    /* 默认初始化为发送模式 */
    HAL_RF_SetTxMode(&hrf);
    /* 初始化发送和接收队列 */
    queue_init(&rf_txQueue, rf_send_queue_buffer, 
                txqueue_item_size*rf_send_queue_len, rf_send_queue_len, txqueue_item_size);
    queue_init(&rf_rxQueue, rf_recv_queue_buffer,
                rxqueue_item_size*rf_recv_queue_len, rf_recv_queue_len, rxqueue_item_size);
    
}

/* 发送数据入队函数
 * data_pack: 指向要发送的数据包指针
 * len: 数据包长度，最大不超过 max_rf_payload_len 32字节
 */
void RF_txQueue_Send(uint8_t *data_pack, uint8_t len)
{
    if(len > max_rf_payload_len) {
        len = max_rf_payload_len; //限制最大长度
    }

    /* 封包，在净荷数据前加上长度 */
    uint8_t temp_data[txqueue_item_size];
    temp_data[0] = len;

    memcpy(&temp_data[1], data_pack, len);
    //入队
    queue_push_overwrite(&rf_txQueue, temp_data);
}

/**
 * @brief  从接收队列取出一帧数据
 * @param   data_ptr:  输出参数，指向接收到的数据指针
 * @param   out_len:   输出参数， 指向接收到的数据长度
 * @param   pipes:     输出参数， 指向接收到的数据管道号
 * @return  1:成功, 0:失败(空)
 */
uint8_t RF_rxQueue_Recv(const uint8_t **data_ptr, uint8_t *out_len, uint8_t *pipes)
{
    HAL_RF_SetRxMode(&hrf); //确保在接收模式

    static uint8_t temp_buf[rxqueue_item_size]; //静态缓冲区，防止指针失效
    if(queue_pop(&rf_rxQueue, temp_buf) == 1) {
        if(out_len) 
            *out_len = temp_buf[0];              // 第一字节为有效长度
        if(pipes)
            *pipes = temp_buf[1];                // 第二字节为接收管道
        if(data_ptr) 
            *data_ptr = &temp_buf[2];            // 指向有效数据
        return 1; // 读取成功
    } else {
        return 0; // 队列为空，读取失败
    }
}


// RF 服务处理函数，放在循环/任务中定时运行
void RF_Service_Handler(RF_HandleTypeDef *hrf)
{
    // peek - try - pop 
    // 处理发送队列
    uint8_t tx_data[txqueue_item_size];

    // 检查队列是否为空，不要直接pop
    if(!queue_is_empty(&rf_txQueue)) {
        
        // 使用Peek看队头数据，但不移除,这样即使硬件忙发送失败，数据也不会丢失
        queue_peek(&rf_txQueue, tx_data);
        if(__HAL_RF_Get_TRxMode_Bit() == 1) {
            HAL_RF_SetTxMode(hrf);
        }
        // uart_printf("TRX_CONFIG_mode:%d\n",TRX_CONFIG);
        // 判断净荷长度
        uint8_t len = tx_data[0]<=max_rf_payload_len ? tx_data[0] : max_rf_payload_len;
        
        // 尝试调用发送函数
        // 如果它返回 BUSY，说明它内部运行了超时检测逻辑但还没发完
        // 如果它返回 OK，说明硬件空闲并接受了新数据。
        if(HAL_RF_Transmit_IT(hrf, &tx_data[1], len) == HAL_OK){
            uart_printf("rf_send_service len:%d\n", len);
            //只有发送成功受理了，才真正把数据从队列里弹出来
            queue_pop(&rf_txQueue, tx_data);
            
        }
    }

    //退出之前确保在接收模式，让中断能正常接收
    HAL_RF_SetRxMode(hrf);

}
