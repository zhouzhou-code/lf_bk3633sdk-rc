#ifndef __RF_HANDLER_H
#define __RF_HANDLER_H

#include "Types.h"
#include "hal_drv_rf.h"
#include "hal_drv_rf.h"
#include "my_queue.h"

#define max_rf_payload_len   32 //RF单次发送最大净荷长度 
#define rf_protocol_overhead 1  //协议开销，目前只占用一字节，存放净荷长度
#define queue_item_size     (max_rf_payload_len + rf_protocol_overhead)  //队列每项大小
#define rf_send_queue_len    20
#define rf_recv_queue_len    20

extern my_queue_t rf_rxQueue; 
extern my_queue_t rf_txQueue;
extern RF_HandleTypeDef hrf; //全局RF句柄

void RF_Handler_Init(void);
void RF_txQueue_Send(uint8_t *data_pack, uint8_t len);
uint8_t RF_rxQueue_Recv(const uint8_t **data_ptr, uint8_t *out_len);
// uint8_t RF_rxQueue_Recv(uint8_t *data_pack, uint8_t len);
void RF_Service_Handler(RF_HandleTypeDef *hrf);

#endif /* __RF_HANDLER_H */
