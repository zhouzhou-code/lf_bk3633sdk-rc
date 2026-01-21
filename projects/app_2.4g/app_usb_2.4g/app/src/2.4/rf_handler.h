#ifndef __RF_HANDLER_H
#define __RF_HANDLER_H

#include "Types.h"
#include "hal_drv_rf.h"
#include "hal_drv_rf.h"
#include "my_queue.h"

#define MAX_RF_PAYLOAD_LEN   32
#define rf_send_queue_len    10
#define rf_recv_queue_len    10

extern my_queue_t rf_rxQueue; 
extern my_queue_t rf_txQueue;
extern RF_HandleTypeDef hrf; //全局RF句柄

void RF_Handler_Init(void);
void RF_txQueue_Send(uint8_t *data_pack, uint8_t len);
uint8_t RF_rxQueue_Recv(uint8_t *data_pack, uint8_t len);
void RF_Send_Handler(RF_HandleTypeDef *hrf);

#endif /* __RF_HANDLER_H */
