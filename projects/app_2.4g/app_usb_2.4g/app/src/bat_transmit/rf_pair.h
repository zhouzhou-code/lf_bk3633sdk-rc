#ifndef __RF_PAIR_H__
#define __RF_PAIR_H__
#include <stdint.h>

#define PAIR_CH_DEFAULT           78
static const uint8_t PAIR_ADDR_DEFAULT[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

static uint8_t slave_magic_number = 0x5A; 
static uint8_t host_magic_number  = 0xA5;


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

typedef struct {
    uint8_t cmd;
    uint8_t new_addr[5];
    uint8_t new_chn;
} pair_resp_pkt;

typedef struct {
    uint8_t cmd;
    uint8_t magic_number;
} pair_confirm_pkt;

#pragma pack()

#endif