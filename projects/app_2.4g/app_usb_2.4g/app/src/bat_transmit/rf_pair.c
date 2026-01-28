#include "driver_bk24.h"
#include "hal_drv_rf.h"
#include "rf_handler.h" 
#include "rf_pair.h" 
#include "user_config.h" 

#include "app.h"
#include "bk3633_reglist.h"
#include "icu.h"
#include "driver_timer.h"
#include <string.h>

#include "intc.h"            // interrupt controller
#include "uart.h"            // uart definitions
#include "BK3633_Reglist.h"
#include "gpio.h"
#include "spi.h"
#include "i2c.h"
#include "icu.h"
#include "dma.h"
#include "reg_intc.h"
#include "driver_timer.h"
#include "app.h"

uint8_t slave_pair_success_flag = 0;
uint8_t host_pair_success_flag = 0;

static const uint32_t PAIR_ADDR_DEFAULT[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

static uint8_t slave_magic_number = 0x5A; 
static uint8_t host_magic_number  = 0xA5;




printf_all_registers(void)
{
    uart_printf("TRX_CONFIG     = 0x%02X\r\n", TRX_CONFIG);
    uart_printf("TRX_EN_AA      = 0x%02X\r\n", TRX_EN_AA);
    uart_printf("TRX_EN_RXADDR  = 0x%02X\r\n", TRX_EN_RXADDR);
    uart_printf("TRX_SETUP_AW   = 0x%02X\r\n", TRX_SETUP_AW);
    uart_printf("TRX_SETUP_RETR = 0x%02X\r\n", TRX_SETUP_RETR);
    uart_printf("TRX_RF_CH      = 0x%02X\r\n", TRX_RF_CH);
    uart_printf("TRX_RF_SETUP   = 0x%02X\r\n", TRX_RF_SETUP);

    uart_printf("TRX_RX_ADDR_P0 = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_RX_ADDR_P0_0)[i]);
    uart_printf("\r\n");

    uart_printf("TRX_RX_ADDR_P1 = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_RX_ADDR_P1_0)[i]);
    uart_printf("\r\n");

    uart_printf("TRX_RX_ADDR_P2 = 0x%02X\r\n", TRX_RX_ADDR_P2);
    uart_printf("TRX_RX_ADDR_P3 = 0x%02X\r\n", TRX_RX_ADDR_P3);
    uart_printf("TRX_RX_ADDR_P4 = 0x%02X\r\n", TRX_RX_ADDR_P4);
    uart_printf("TRX_RX_ADDR_P5 = 0x%02X\r\n", TRX_RX_ADDR_P5);

    uart_printf("TRX_TX_ADDR    = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_TX_ADDR_0)[i]);
    uart_printf("\r\n");

    uart_printf("TRX_RX_PW_P0   = 0x%02X\r\n", TRX_RX_PW_P0);
    uart_printf("TRX_RX_PW_P1   = 0x%02X\r\n", TRX_RX_PW_P1);
    uart_printf("TRX_RX_PW_P2   = 0x%02X\r\n", TRX_RX_PW_P2);
    uart_printf("TRX_RX_PW_P3   = 0x%02X\r\n", TRX_RX_PW_P3);
    uart_printf("TRX_RX_PW_P4   = 0x%02X\r\n", TRX_RX_PW_P4);
    uart_printf("TRX_RX_PW_P5   = 0x%02X\r\n", TRX_RX_PW_P5);

    uart_printf("TRX_DYNPD      = 0x%02X\r\n", TRX_DYNPD);
    uart_printf("TRX_FEATURE    = 0x%02X\r\n", TRX_FEATURE);

    uart_printf("addXVR_Reg0x24 = 0x%08lX\r\n", addXVR_Reg0x24);
    uart_printf("addXVR_Reg0x3b = 0x%08lX\r\n", addXVR_Reg0x3b);
    uart_printf("addXVR_Reg0x2e = 0x%08lX\r\n", addXVR_Reg0x2e);
    uart_printf("addXVR_Reg0x26 = 0x%08lX\r\n", addXVR_Reg0x26);
    uart_printf("addXVR_Reg0x2  = 0x%08lX\r\n", addXVR_Reg0x2);
    uart_printf("addXVR_Reg0x2c = 0x%08lX\r\n", addXVR_Reg0x2c);
    uart_printf("addXVR_Reg0x2d = 0x%08lX\r\n", addXVR_Reg0x2d);
    uart_printf("addXVR_Reg0x3a = 0x%08lX\r\n", addXVR_Reg0x3a);
}



RF_HandleTypeDef h_pair; //配对专用RF句柄
RF_ConfgTypeDef Pairing_Config =
{
    .Mode = MODE_TX,
    .DataRate = BPS_2M,
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
    printf_all_registers();
    HAL_RF_SetTxMode(&h_pair);
    uart_printf("Slave: Start Pairing...\n");
    
    while(1) {

        //发请求:阻塞发送,直到收到Host的ACK
        if (HAL_RF_Transmit_ACK(&h_pair, (uint8_t*)&pkt, sizeof(pkt)) == HAL_RF_STATE_READY) {
            //立即切接收，等Host的回复包
            HAL_RF_SetRxMode(&h_pair);
            uart_printf("Slave:set to rx for resp...\n");
            uint32_t start_wait = Get_SysTick_ms();
            uint8_t len;
            uint8_t rx_buf[sizeof(pair_resp_pkt)];
            while (Get_SysTick_ms() - start_wait < 500) { //最大轮询500ms是否有数据
                uart_printf("while1:%d\r\n",Get_SysTick_ms() - start_wait);
                if (HAL_RF_Receive(&h_pair, rx_buf, &len) == HAL_OK) {
                    uart_printf("rec_resp_ok\r\n");

                    if(len == sizeof(pair_resp_pkt))
                        resp = (pair_resp_pkt*)rx_buf;

                    if (resp->cmd == CMD_PAIR_RESP) {
                        uart_printf("Slave: Received Pair Resp!\n");
                        uart_printf("Slave: Got New Config! Ch:%d\n", resp->new_chn);
                        uart_printf("Slave: New Addr: %02X %02X %02X %02X %02X\n",
                            resp->new_addr[0], resp->new_addr[1], resp->new_addr[2],
                            resp->new_addr[3], resp->new_addr[4]);
                        got_resp = 1;
                        break;
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
                                slave_pair_success_flag = 1;
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
    printf_all_registers();
    HAL_RF_SetRxMode(&h_pair);

    uart_printf("Host: Start Pairing (Waiting for Req...)\n");

    while(1) {
        //在默认信道等REQ
        if (pair_step == 0) {
            if (HAL_RF_Receive(&h_pair, rx_buf, &len) == HAL_OK) {
                if (len == sizeof(pair_req_pkt)) {
                    req = (pair_req_pkt*)rx_buf;
                    if (req->cmd == CMD_PAIR_REQ) {
                        uart_printf("Host: Received Pair Req from %08X\n", req->slave_id);

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
                    host_pair_success_flag = 1;
                    return;
                } else {
                    uart_printf("Host: Final ACK failed, but Slave is on site. Done.\n");
                    host_pair_success_flag = 1;
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

    



void Do_Pairing_As_Host_it(void) {
    pair_resp_pkt resp; 
    pair_confirm_pkt host_cfm = {CMD_PAIR_CONFIRM, host_magic_number}; //回复给Slave的确认包
    pair_req_pkt *req; // 接收请求包指针
    pair_confirm_pkt *rec_slave; // 接收Slave confirm包指针
    
    uint8_t len;
    uint8_t rx_buf[32]; // 通用接收缓冲
    static uint8_t pair_step = 0; // 0:等REQ, 1:等CONFIRM

    // 初始化为默认配对配置并进入接收模式
    
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxMode(&hrf);
    printf_all_registers();
    uart_printf("Host: Start Pairing (Waiting for Req...)\n");

    while(1) {
        
    }
}



// void Do_Pairing_As_slave_it(void) {

//     pair_req_pkt req_pkt={CMD_PAIR_REQ,0x12345678}; //发送配对请求包
//     pair_confirm_pkt slave_cfm_pkg={CMD_PAIR_CONFIRM,slave_magic_number};//发送配对确认包
    
//     pair_resp_pkt *recv_resp_pkg; //接收配对响应包
//     pair_confirm_pkt *recv_cfm_pkg;//接收confirm确认包

//     static uint8_t got_resp = 0;
//     HAL_RF_Init(&h_pair,&Pairing_Config);
//     printf_all_registers();
//     HAL_RF_SetTxMode(&h_pair);
//     uart_printf("Slave: Start Pairing...\n");

//     // 初始化为默认配对配置并进入接收模式
    
//     HAL_RF_SetTxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
//     HAL_RF_SetTxMode(&hrf);
//     printf_all_registers();
//     uart_printf("Slave: Start Pairing (Waiting for Req...)\n");
   
//     while(1) {
        
//         //发送req
//         RF_txQueue_Send(&hrf, (uint8_t*)&req_pkt, sizeof(req_pkt));
//         RF_Service_Handler(&hrf);

//         //阻塞等待resp,从接收队列取数据，直到取到或者超时
//         uint32_t start_wait = Get_SysTick_ms();
//         uint8_t len;
//         while (Get_SysTick_ms() - start_wait < 500)  {
//             if (RF_rxQueue_Recv(&recv_resp_pkg, &len, NULL) == 1) { 
//                 if (len == sizeof(pair_resp_pkt) && recv_resp_pkg->cmd == CMD_PAIR_RESP) {
//                     break;
//                 }
//             }
//         }




//     }
// }
static printf_txrx_addr()
{
    uart_printf("TRX_RX_ADDR_P0 = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_RX_ADDR_P0_0)[i]);
    uart_printf("\r\n");

    uart_printf("TRX_TX_ADDR    = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_TX_ADDR_0)[i]);
    uart_printf("\r\n");
}


void Do_Pairing_As_slave_SM(void) {
    slave_pair_state_t state = SLAVE_PAIR_IDLE;
    pair_req_pkt req_pkt = {CMD_PAIR_REQ, 0x12345678};
    pair_resp_pkt *recv_resp_pkg = NULL;
    pair_confirm_pkt slave_cfm_pkg = {CMD_PAIR_CONFIRM, slave_magic_number};
    pair_confirm_pkt *recv_cfm_pkg = NULL;
    uint8_t len;
    uint32_t start_wait;
    const uint16_t RESP_WAIT_TIMEOUT = 3000; // ms
    const uint16_t CONFIRM_WAIT_TIMEOUT = 3000;
    uint8_t confirm_retries = 0; // 添加confirm重发计数
    const uint8_t MAX_CONFIRM_RETRIES = 3; // 最大重发次数

    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    
    HAL_RF_SetTxMode(&hrf);
    printf_all_registers();
    uart_printf("Slave: Start Pairing (send Req...)\n");

    while (state != SLAVE_PAIR_DONE) {
        
        switch (state) {
        case SLAVE_PAIR_IDLE:
            uart_printf("idle\n");
            state = SLAVE_PAIR_SEND_REQ;
            break;

        case SLAVE_PAIR_SEND_REQ:
            uart_printf("state:send req\n");

            RF_txQueue_Send((uint8_t*)&req_pkt, sizeof(req_pkt));
            RF_Service_Handler(&hrf);
            start_wait = Get_SysTick_ms();
            state = SLAVE_PAIR_WAIT_RESP;
            break;

        case SLAVE_PAIR_WAIT_RESP:
            uart_printf("state:wait resp\n");
            while (Get_SysTick_ms() - start_wait < RESP_WAIT_TIMEOUT) {
                if (RF_rxQueue_Recv(&recv_resp_pkg, &len, NULL) == 1) {
                    uart_printf("check resp len:%d\r\n",len);
                    if (len == sizeof(pair_resp_pkt) && recv_resp_pkg->cmd == CMD_PAIR_RESP) {
                        
                        //将uint8_t拓展成uint32_t数组
                        uint32_t new_addr[5];
                        for(int i=0;i<5;i++)
                            new_addr[i] = recv_resp_pkg->new_addr[i];

                        //切换到新地址
                        HAL_RF_SetTxAddress(&hrf, new_addr, 5);
                        HAL_RF_SetRxAddress(&hrf, 0, new_addr, 5);
                        
                        uart_printf("Slave: new_addr!\n");
                        printf_txrx_addr();
                        state = SLAVE_PAIR_SEND_CONFIRM;
                        break;
                    }
                }
            }
            if (state != SLAVE_PAIR_SEND_CONFIRM) {
                uart_printf("Wait Resp To\n");
                state = SLAVE_PAIR_SEND_REQ;
            }
            break;

        case SLAVE_PAIR_SEND_CONFIRM:
            uart_printf("state:send confirm (%d)\n", confirm_retries + 1);
            RF_txQueue_Send((uint8_t*)&slave_cfm_pkg, sizeof(slave_cfm_pkg));
            HAL_RF_SetTxMode(&hrf);
            RF_Service_Handler(&hrf);

            confirm_retries++; //递增重发计数
            start_wait = Get_SysTick_ms();
            state = SLAVE_PAIR_WAIT_CONFIRM_ACK;
            break;

        case SLAVE_PAIR_WAIT_CONFIRM_ACK:
            uart_printf("state:wait confirm ack\n");
            HAL_RF_SetRxMode(&hrf);
            while (Get_SysTick_ms() - start_wait < CONFIRM_WAIT_TIMEOUT) {
                if (RF_rxQueue_Recv(&recv_cfm_pkg, &len, NULL) == 1) {
                    uart_printf("check confirm len:%d\r\n",len);
                    if (len == sizeof(pair_confirm_pkt) &&
                        recv_cfm_pkg->cmd == CMD_PAIR_CONFIRM &&
                        recv_cfm_pkg->magic_number == host_magic_number) {
                        uart_printf("Slave: Pairing Confirmed!\n");
                        slave_pair_success_flag = 1;
                        confirm_retries = 0; // 成功后重置计数
                        state = SLAVE_PAIR_DONE;
                        break;
                    }
                }
            }
            if (state != SLAVE_PAIR_DONE) {
                if (confirm_retries < MAX_CONFIRM_RETRIES) {
                    uart_printf("Slave: Confirm No Resp, resend confirm (%d/%d)\n", confirm_retries, MAX_CONFIRM_RETRIES);
                    state = SLAVE_PAIR_SEND_CONFIRM; // 重发confirm
                } else {
                    uart_printf("Slave: Confirm failed after %d retries, restart pairing\n", confirm_retries);
                    confirm_retries = 0; // 重置计数
                    state = SLAVE_PAIR_SEND_REQ; // 回到发送REQ
                    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
                    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
                }
            }
            break;

        default:
            uart_printf("Slave: Unknown State, resetting...\n");
            state = SLAVE_PAIR_IDLE;
            break;
        }
    }

    while(state == HOST_PAIR_DONE){
        uart_printf("slave: pairing process completed\n");
        printf_txrx_addr();
    }
    
}



void Do_Pairing_As_Host_SM(void) {
    host_pair_state_t state = HOST_PAIR_IDLE;
    pair_resp_pkt resp;
    pair_confirm_pkt host_cfm = {CMD_PAIR_CONFIRM, host_magic_number};
    pair_req_pkt *recv_req = NULL;
    pair_confirm_pkt *recv_cfm = NULL;
    uint8_t len;
    uint32_t start_wait;
    uint8_t resp_retries = 0;
    const uint8_t MAX_RESP_RETRIES = 3;
    const uint16_t CONFIRM_WAIT_TIMEOUT = 500; //ms

    //初始化并监听默认地址
    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    printf_all_registers();
    HAL_RF_SetRxMode(&hrf);
    uart_printf("Host: Start Pairing (Waiting for Req...)\n");
    state = HOST_PAIR_WAIT_REQ;

    while (state != HOST_PAIR_DONE) {
        switch (state) {

        case HOST_PAIR_WAIT_REQ:
            if (RF_rxQueue_Recv(&recv_req, &len, NULL) == 1) {
                if (len == sizeof(pair_req_pkt) && recv_req->cmd == CMD_PAIR_REQ) {
                    uart_printf("Host: Received Pair Req from %08X\n", recv_req->slave_id);
                    
                    resp.cmd = CMD_PAIR_RESP;
                    resp.new_chn = 20;
                    resp.new_addr[0] = 0x11; resp.new_addr[1] = 0x22;
                    resp.new_addr[2] = 0x33; resp.new_addr[3] = 0x44; resp.new_addr[4] = 0x55;
                    
                    state = HOST_PAIR_SEND_RESP;
                }
            }
            break;

        case HOST_PAIR_SEND_RESP:
            uart_printf("Host: Sending RESP (try %d)\n", resp_retries+1);
            HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
            
            RF_txQueue_Send((uint8_t*)&resp, sizeof(resp));
            RF_Service_Handler(&hrf);
            resp_retries++;

            //切到新地址并进入接收CONFIRM
            uint32_t new_addr[5];
            for(int i=0;i<5;i++)
                new_addr[i] = resp.new_addr[i];
            HAL_RF_SetTxAddress(&hrf, new_addr, 5);
            HAL_RF_SetRxAddress(&hrf, 0, new_addr, 5);
            
            HAL_RF_SetRxMode(&hrf);
            printf_txrx_addr();
            start_wait = Get_SysTick_ms();
            state = HOST_PAIR_WAIT_CONFIRM;
            break;

        case HOST_PAIR_WAIT_CONFIRM:
            uart_printf("Host: Waiting for Slave Confirm...\n");
            while (Get_SysTick_ms() - start_wait < CONFIRM_WAIT_TIMEOUT) {
                if (RF_rxQueue_Recv(&recv_cfm, &len, NULL) == 1) {
                    uart_printf("Host: Checking received packet...\n");
                    if (len == sizeof(pair_confirm_pkt) &&
                        recv_cfm->cmd == CMD_PAIR_CONFIRM &&
                        recv_cfm->magic_number == slave_magic_number) {
                        uart_printf("Host: Received Slave Confirm!\n");
                        state = HOST_PAIR_SEND_FINAL_CONFIRM;
                        break;
                    }
                }
            }
            if (state != HOST_PAIR_SEND_FINAL_CONFIRM) { //如果没收到slave的confirm
                if (resp_retries < MAX_RESP_RETRIES) { //如果没达到最大重发次数，回到发送RESP状态重试
                    uart_printf("Host: Confirm Timeout, resend RESP\n");
                    state = HOST_PAIR_SEND_RESP;
                } else { //超次数了，放弃配对，恢复默认配置
                    uart_printf("Host: Confirm Timeout after %d tries, revert to default\n", resp_retries);
                    resp_retries = 0;
                    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
                    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
                    HAL_RF_SetRxMode(&hrf);
                    state = HOST_PAIR_WAIT_REQ;
                }
            }
            break;

        case HOST_PAIR_SEND_FINAL_CONFIRM:
            uart_printf("Host: Sending final confirm to Slave\n");
            RF_txQueue_Send((uint8_t*)&host_cfm, sizeof(host_cfm));
            RF_Service_Handler(&hrf);
            host_pair_success_flag = 1;
            uart_printf("Host: Pairing Success!\n");
            state = HOST_PAIR_DONE;
            break;

        default:
            uart_printf("Host: Unknown State, resetting...\n");
            state = HOST_PAIR_IDLE;
            break;
        }
    }

    while(state == HOST_PAIR_DONE){
        uart_printf("Host: pairing process completed\n");
        printf_txrx_addr();
    }
        
}
