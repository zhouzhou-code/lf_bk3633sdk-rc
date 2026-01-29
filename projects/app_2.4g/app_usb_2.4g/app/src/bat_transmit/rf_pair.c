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
#include "spi.h"
#include "i2c.h"
#include "icu.h"
#include "dma.h"
#include "reg_intc.h"
#include "driver_timer.h"
#include "app.h"

#include "addr_pool.h"
#include "timer_handler.h"


uint8_t slave_pair_success_flag = 0;
uint8_t host_pair_success_flag = 0;

static const uint32_t PAIR_ADDR_DEFAULT[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

static uint8_t slave_magic_number = 0x5A; 
static uint8_t host_magic_number  = 0xA5;

SingleByteAddrPool_t g_addr3;
SingleByteAddrPool_t g_addr4;


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

static printf_txrx_addr()
{
    uart_printf("TRX_RX_ADDR_P0 = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_RX_ADDR_P0_0)[i]);
    uart_printf("\r\n");

    uart_printf("TRX_TX_ADDR    = ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_TX_ADDR_0)[i]);
    uart_printf("\r\n");
}


//传入1表示停止配对
void Do_Pairing_As_slave_SM() {
    slave_pair_state_t state = SLAVE_PAIR_IDLE;//状态机
    pair_req_pkt req_pkt = {CMD_PAIR_REQ, 0x12345678};//slave请求配对包
    pair_resp_pkt *recv_resp_pkg = NULL;//接收host响应包
    pair_confirm_pkt slave_cfm_pkg = {CMD_PAIR_CONFIRM, slave_magic_number};//slave确认包
    pair_confirm_pkt *recv_cfm_pkg = NULL;//接收host确认包
    
    uint8_t len;
    uint32_t start_wait;
    const uint16_t RESP_WAIT_TIMEOUT = 1000; //ms;不能太短
    const uint16_t CONFIRM_WAIT_TIMEOUT = 1000;//ms;不能太短
    uint8_t confirm_retries = 0; //confirm发送计数
    const uint8_t MAX_CONFIRM_RETRIES = 3; // confirm最大重发次数

    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    
    HAL_RF_SetTxMode(&hrf);
    printf_all_registers();
    uart_printf("Slave: Start Pairing (send Req...)\n");

    while (state != SLAVE_PAIR_DONE ) {
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
    // if(state == SLAVE_PAIR_DONE){
    //     uart_printf("slave: pairing process completed\n");
    //     printf_txrx_addr();
    //     return ;
    // }
    // if(stop_signal){
    //     uart_printf("slave: pairing process stopped by signal\n");
    //     return ;
    // }
    
}


void Do_Pairing_As_Host_SM(void) {
    host_pair_state_t state = HOST_PAIR_IDLE; //host状态机
    pair_resp_pkt resp; //host响应req包
    pair_confirm_pkt host_cfm = {CMD_PAIR_CONFIRM, host_magic_number};//host响应confirm包
    pair_req_pkt *recv_req = NULL; //接收slave请求包
    pair_confirm_pkt *recv_cfm = NULL;//接收slave确认包
    
    uint8_t len;
    uint32_t start_wait;
    uint8_t resp_retries = 0;//响应req包重发计数
    const uint8_t MAX_RESP_RETRIES = 3;//响应req包最大重发次数
    const uint16_t CONFIRM_WAIT_TIMEOUT = 1000; //ms

    /* 初始化ID池 */
    addrpool_register_get_systick_ms(Get_SysTick_ms);
    ADDR_POOL_INIT_NORESERVED(&g_addr3);
    ADDR_POOL_INIT_NORESERVED(&g_addr4);

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
                    resp.new_addr[0] = ADDR_BASE_BYTE0;
                    resp.new_addr[1] = ADDR_BASE_BYTE1;
                    resp.new_addr[2] = ADDR_BASE_BYTE2; 
                    // resp.new_addr[3] = ADDR_BASE_BYTE3; 
                    if(addrpool_alloc_addr_random(&g_addr3, &resp.new_addr[3]) != 0) {
                        uart_printf("Failed to allocate random address\n");
                        // Handle error, maybe set state to an error state or retry
                    }
                    if(addrpool_alloc_addr_random(&g_addr4, &resp.new_addr[4]) != 0) {
                        uart_printf("Failed to allocate random address\n");
                        // Handle error, maybe set state to an error state or retry
                    }
                    
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
            start_wait = Get_SysTick_ms();
            uint32_t txds_record = rf_int_count_txds; //记录发送成功中断计数
            
            while(Get_SysTick_ms()-start_wait < 1000){ //连续发1秒，确保slave收到
                RF_txQueue_Send((uint8_t*)&host_cfm, sizeof(host_cfm));
                RF_Service_Handler(&hrf);
                //检查发送成功中断计数是否增加，增加则说明发送成功
                if(rf_int_count_txds >= txds_record){
                    state = HOST_PAIR_DONE;
                    uart_printf("Host: Pairing Success:cur_tx%d,tx_record%d\n", rf_int_count_txds, txds_record);
                    break;
                }
            }
        //超时，slave未收到confirm确认包;slave可能在重发comfirm或者在重发req，hsot直接回退到等待req状态重新开始配对
            if(state != HOST_PAIR_DONE){
                uart_printf("Host: Final Confirm send failed, restarting pairing\n");
                HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
                HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
                HAL_RF_SetRxMode(&hrf);
                state = HOST_PAIR_WAIT_REQ;
            }
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
