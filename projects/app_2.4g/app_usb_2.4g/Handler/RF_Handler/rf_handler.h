#ifndef __RF_HANDLER_H
#define __RF_HANDLER_H

#include "Types.h"
#include "hal_drv_rf.h"
#include "hal_drv_rf.h"
#include "my_queue.h"

#define max_rf_payload_len   32 //RF单次发送最大净荷长度 
#define rf_protocol_overhead 1  //协议开销，占用1字节
//发送队列每项大小，净荷长度+净荷数据;RF通过净荷长度发送不定长的净荷数据
#define txqueue_item_size     (max_rf_payload_len + rf_protocol_overhead)  
//接收队列每项大小，净荷长度+接收pipes+净荷数据;RF接收中断会判断长度和pipes并补上压入qeueue
#define rxqueue_item_size     (max_rf_payload_len + 1 + rf_protocol_overhead) 
#define rf_send_queue_len    64
#define rf_recv_queue_len    64

extern my_queue_t rf_rxQueue; 
extern my_queue_t rf_txQueue;
extern RF_HandleTypeDef hrf; //全局RF句柄

extern uint32_t rf_int_count_rxdr;
extern uint32_t rf_int_count_txds;
extern uint32_t rf_int_count_maxrt;

void RF_Handler_Init(void);
void RF_txQueue_Send(uint8_t *data_pack, uint8_t len);
void RF_txQueue_Clear(void);
uint8_t RF_rxQueue_Recv(const uint8_t **data_ptr, uint8_t *out_len, uint8_t *pipes);
uint8_t RF_rxQueue_clear(void);
void RF_Service_Handler(RF_HandleTypeDef *hrf);

#endif /* __RF_HANDLER_H */
