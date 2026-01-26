#include "driver_bk24.h"
#include "hal_drv_rf.h"
#include "rf_handler.h" 
#include "rf_pair.h" 
#include "user_config.h" 

RF_HandleTypeDef h_pair; //配对专用RF句柄

RF_ConfgTypeDef Pairing_Config =
{
    .Mode = MODE_TX,
    .DataRate = BPS_250K,
    .TxPower = RF_TX_POWER_0dBm,
    .Channel = PAIR_CH_DEFAULT, 
    .Protocol ={
        .AddressWidth = 5,
        .TxAddress = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE},
        .Support_NoAck = 1,             
        .AutoRetransmitDelay = 2,        //最大重传延迟 750us
        .AutoRetransmitCount = 3,        //最大重传次数 3
        .RxPipes[0] = {
            .PipeNum = 0,
            .Enable = 1,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}
        },
        .RxPipes[1] = {
            .PipeNum = 1,
            .Enable = 0,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x10, 0x56, 0x24, 0xCD, 0x78}
        },
        .RxPipes[2]={
            .PipeNum = 2,
            .Enable = 0,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x11, 0, 0, 0, 0}
        },
        .RxPipes[3]={
            .PipeNum = 3,
            .Enable = 0,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x12, 0, 0, 0, 0}
        },
        .RxPipes[4]={
            .PipeNum = 4,
            .Enable = 0,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x13, 0, 0, 0, 0}
        },
        .RxPipes[5]={
            .PipeNum = 5,
            .Enable = 0,
            .AutoAck = 1,
            .EnDynamicPayload = 1,
            .PayloadWidth = 32,
            .Support_Payload_Attach_ACK = 1,
            .Address = {0x14, 0, 0, 0, 0}
        }
    },
    .IRQ = {
        .RxDR = {
            .enable = 0,
            .user_cb = NULL
        },
        .TxDS = {
            .enable = 0,
            .user_cb = NULL
        },
        .MaxRT = {
            .enable = 0,
            .user_cb = NULL
        }
    }
};



void Do_Pairing_As_Slave(void) {
    pair_req_pkt pkt={CMD_PAIR_REQ,0x12345678}; //发送配对请求包
    pair_confirm_pkt slave_cfm={CMD_PAIR_CONFIRM,slave_magic_number};//发送配对确认包
    pair_resp_pkt *resp; //接收配对响应包
    pair_confirm_pkt *rec_host;//接收confirm确认包

    static uint8_t got_resp = 0;
    HAL_RF_Init(&h_pair,&Pairing_Config);
    HAL_RF_SetTxMode(&h_pair);

    uart_printf("Slave: Start Pairing...\n");

    while(1) {

        //发请求:阻塞发送,直到收到Host的ACK
        if (HAL_RF_Transmit_ACK(&h_pair, (uint8_t*)&pkt, sizeof(pkt)) == HAL_RF_STATE_READY) {
            //立即切接收，等Host的回复包
            HAL_RF_SetRxMode(&h_pair);
            uint32_t start_wait = Get_SysTick_ms();
            while (Get_SysTick_ms() - start_wait < 500) { //轮询500ms是否有数据
                uint8_t len;
                uint8_t rx_buf[sizeof(pair_resp_pkt)];
                if (HAL_RF_Receive(&h_pair, rx_buf, &len) == HAL_OK) {
                    if(len == sizeof(pair_resp_pkt))
                        resp = (pair_resp_pkt*)rx_buf;

                    if (resp->cmd == CMD_PAIR_RESP) {
                        uart_printf("Slave: Received Pair Resp!\n");
                        uart_printf("Slave: Got New Config! Ch:%d\n", resp->new_chn);
                        uart_printf("Slave: New Addr: %02X %02X %02X %02X %02X\n",
                            resp->new_addr[0], resp->new_addr[1], resp->new_addr[2],
                            resp->new_addr[3], resp->new_addr[4]);
                        got_resp = 1;
                        return;
                    }
                }
            }
            HAL_RF_SetTxMode(&h_pair);//没数据立刻切出去重发
            uart_printf("Slave: req No Resp, Retry...\n");
        }

        // 如果收到了 RESP，发送 CONFIRM 并保存配置
        if (got_resp==1) {
                //按照新地址和信道配置发送confirm
                HAL_RF_SetTxAddress(&h_pair, resp->new_addr, 5);
                HAL_RF_SetRxAddress(&h_pair, 0, resp->new_addr, 5);
                HAL_RF_SetTxMode(&h_pair);
                
                if (HAL_RF_Transmit_ACK(&h_pair, (uint8_t*)&slave_cfm, sizeof(slave_cfm)) == HAL_RF_STATE_READY) {
                    HAL_RF_SetRxMode(&h_pair);
                    uint32_t start_wait = Get_SysTick_ms();
                    while (Get_SysTick_ms() - start_wait < 500) { //轮询500ms是否有数据
                        uint8_t len;
                        uint8_t rx_buf[sizeof(pair_confirm_pkt)];
                        if (HAL_RF_Receive(&h_pair, rx_buf, &len) == HAL_OK) {
                            if(len == sizeof(pair_confirm_pkt))
                                rec_host = (pair_confirm_pkt*)rx_buf;
                            
                            //comfirm包收发成功，保存配置并退出配对
                            if (rec_host->cmd == CMD_PAIR_CONFIRM && rec_host->magic_number == host_magic_number) {
                                uart_printf("Slave: Pairing Confirmed!\n");
                                got_resp = 2;
                                return;
                            }
                        }
                    }
                    HAL_RF_SetTxMode(&h_pair);//没数据立刻切出去重发
                    uart_printf("Slave: Confirm No Resp, Retry...\n");
                }
        }
    }
}

void Do_Pairing_As_Host(void) {
    pair_resp_pkt resp; 
    pair_confirm_pkt host_cfm = {CMD_PAIR_CONFIRM, host_magic_number}; //回复给Slave的确认包
    pair_req_pkt *req; // 接收请求包指针
    pair_confirm_pkt *rec_slave; // 接收Slave confirm包指针
    
    uint8_t len;
    uint8_t rx_buf[32]; // 通用接收缓冲
    static uint8_t pair_step = 0; // 0:等REQ, 1:等CONFIRM

    // 初始化为默认配对配置并进入接收模式
    HAL_RF_Init(&h_pair, &Pairing_Config);
    HAL_RF_SetRxMode(&h_pair);

    uart_printf("Host: Start Pairing (Waiting for Req...)\n");

    while(1) {
        //在默认信道等REQ
        if (pair_step == 0) {
            if (HAL_RF_Receive(&h_pair, rx_buf, &len) == HAL_OK) {
                if (len == sizeof(pair_req_pkt)) {
                    req = (pair_req_pkt*)rx_buf;
                    if (req->cmd == CMD_PAIR_REQ) {
                        uart_printf("Host: Received Pair Req from %08X\n", req->uid);

                        // 构造响应包参数
                        resp.cmd = CMD_PAIR_RESP;
                        resp.new_chn = 20; //拟分配信道
                        // 拟分配地址
                        resp.new_addr[0] = 0x11; resp.new_addr[1] = 0x22;
                        resp.new_addr[2] = 0x33; resp.new_addr[3] = 0x44;
                        resp.new_addr[4] = 0x55;

                        // 发送 RESP必须切到发送模式
                        HAL_RF_SetTxMode(&h_pair);
                        if (HAL_RF_Transmit_ACK(&h_pair, (uint8_t*)&resp, sizeof(resp)) == HAL_RF_STATE_READY) {
                            uart_printf("Host: Resp Sent. Switching to New Addr...\n");

                            //切换到新地址等 CONFIRM 
                            HAL_RF_SetTxAddress(&h_pair, resp.new_addr, 5);
                            HAL_RF_SetRxAddress(&h_pair, 0, resp.new_addr, 5);
                            HAL_RF_SetRxMode(&h_pair); 

                            pair_step = 1;
                        } else {
                            // 发送RESP失败,没收到 Slave 的 ACK，切回接收继续等
                            HAL_RF_SetRxMode(&h_pair);
                            uart_printf("Host: Send Resp Failed, Retry Wait REQ...\n");
                        }
                    }
                }
            }
        }
        //在新信道等Slave发来的CONFIRM
        else if (pair_step == 1) {
            uint32_t start_wait = Get_SysTick_ms();
            uint8_t got_cfm = 0;

            // 在新信道轮询一段时间
            while (Get_SysTick_ms() - start_wait < 1000) {
                if (HAL_RF_Receive(&h_pair, rx_buf, &len) == HAL_OK) {
                    if (len == sizeof(pair_confirm_pkt)) {
                        rec_slave = (pair_confirm_pkt*)rx_buf;
                        if (rec_slave->cmd == CMD_PAIR_CONFIRM && rec_slave->magic_number == slave_magic_number) {
                            uart_printf("Host: Received Slave Confirm!\n");
                            got_cfm = 1;
                            break;
                        }
                    }
                }
            }

            if (got_cfm) {
                // 收到 Slave 确认后，Host也发送一帧最终确认包给 Slave
                HAL_RF_SetTxMode(&h_pair);
                if (HAL_RF_Transmit_ACK(&h_pair, (uint8_t*)&host_cfm, sizeof(host_cfm)) == HAL_RF_STATE_READY) {
                    uart_printf("Host: Final Confirm Sent. Pairing Success!\n");
                    // 保存配置到 Flash
                    return;
                } else {
                    uart_printf("Host: Final ACK failed, but Slave is on site. Done.\n");
                    return; 
                }
            } else {
                //超时没等到 Slave 的 CONFIRM，认为 Slave 没切过来，回退到默认地址
                uart_printf("Host: Confirm Timeout. Reverting to Default...\n");
                HAL_RF_Init(&h_pair, &Pairing_Config); //恢复默认配置
                HAL_RF_SetRxMode(&h_pair);
                pair_step = 0;
            }
        }
    }
}

    
