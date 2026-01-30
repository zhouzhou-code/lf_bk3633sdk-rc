#ifndef __RF_PAIR_H__
#define __RF_PAIR_H__
#include <stdint.h>

#define PAIR_CH_DEFAULT           78




typedef enum {
    SLAVE_PAIR_IDLE = 0,
    SLAVE_PAIR_SEND_REQ,
    SLAVE_PAIR_WAIT_RESP,
    SLAVE_PAIR_SEND_CONFIRM,
    SLAVE_PAIR_WAIT_CONFIRM_TX_DONE,
    SLAVE_PAIR_WAIT_CONFIRM_ACK,
    SLAVE_PAIR_DONE
} slave_pair_state_t;


typedef enum {
    HOST_PAIR_IDLE = 0,
    HOST_PAIR_WAIT_REQ,
    HOST_PAIR_SEND_RESP,
    HOST_PAIR_WAIT_RESP_TX_DONE,
    HOST_PAIR_WAIT_CONFIRM,
    HOST_PAIR_SEND_FINAL_CONFIRM,
    HOST_PAIR_DONE
} host_pair_state_t;


typedef enum {
    CMD_PAIR_REQ = 0x10,  // Slave -> Host
    CMD_PAIR_RESP = 0x20, // Host -> Slave 

    CMD_PAIR_CONFIRM= 0x30  // Slave <-> Host
} rf_pair_cmd_t;

#pragma pack(1)
typedef struct {
    uint8_t cmd;
    uint32_t slave_id;
} pair_req_pkt;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t cmd;
    uint8_t new_addr[5]; // 新地址，接收方要每字节扩展成uint32_t，发送为了节约按uint8_t发
    uint8_t new_chn;
} pair_resp_pkt;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t cmd;
    uint8_t magic_number;
} pair_confirm_pkt;
#pragma pack()

extern uint8_t slave_pair_success_flag ;
extern uint8_t host_pair_success_flag ;

void Do_Pairing_As_Slave(void);
void Do_Pairing_As_Host(void);

void Do_Pairing_As_Host_SM(void);
void Do_Pairing_As_slave_SM(void);


//非阻塞的配对模式
void Slave_Pairing_Start(void);
void Slave_Pairing_Stop(void);
void Slave_Pairing_Task(void);

void Host_Pairing_Start(void);
void Host_Pairing_Stop(void);
void Host_Pairing_Task(void);

#endif // __RF_PAIR_H__