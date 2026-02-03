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


static void printf_all_registers(void)
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



// 定义状态机管理的结构体，方便重置和管理
typedef struct {
    slave_pair_state_t state;
    uint32_t start_wait;
    uint8_t confirm_retries;
    uint8_t is_running;
    uint32_t txds_snapshot;

    uint8_t verify_cnt;
} SlavePairCtrl_t;

static SlavePairCtrl_t g_slave_ctrl = {SLAVE_PAIR_IDLE, 0, 0, 0, 0};
// 用于外部启动配对
void Slave_Pairing_Start(void) {
    g_slave_ctrl.state = SLAVE_PAIR_IDLE;
    g_slave_ctrl.confirm_retries = 0;
    g_slave_ctrl.is_running = 1;
    
    //初始化RF设置
    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetTxMode(&hrf);
    uart_printf("Slave: Start Pairing Task...\n");
}

//用于外部强制停止配对
void Slave_Pairing_Stop(void) {
    if (g_slave_ctrl.is_running) {
        g_slave_ctrl.is_running = 0;
        g_slave_ctrl.state = SLAVE_PAIR_IDLE;
        uart_printf("Slave: Pairing Process Stopped by External Signal!\n");
    }
}

//非阻塞，需在main while(1)中调用
void Slave_Pairing_Task(void) {
    //如果没启动，直接返回
    if (g_slave_ctrl.is_running == 0) {
        return;
    }

    //局部变量定义
    pair_req_pkt req_pkt = {CMD_PAIR_REQ, 0x12345678};
    pair_resp_pkt *recv_resp_pkg = NULL;
    pair_confirm_pkt slave_cfm_pkg = {CMD_PAIR_CONFIRM, slave_magic_number};
    pair_confirm_pkt *recv_cfm_pkg = NULL;
    pair_confirm_pkt pong_pkt = {CMD_PAIR_VERIFY_PONG, slave_magic_number};
    pair_confirm_pkt *recv_ping_pkt = NULL;
    uint8_t len;
    
    const uint16_t RESP_WAIT_TIMEOUT = 2000; 
    const uint16_t CONFIRM_WAIT_TIMEOUT = 2000;
    const uint8_t MAX_CONFIRM_RETRIES = 3; 

    //必须保证RF底层处理函数被频繁调用
    RF_Service_Handler(&hrf);

    switch (g_slave_ctrl.state) {
        case SLAVE_PAIR_IDLE:
        g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
        break;

    case SLAVE_PAIR_SEND_REQ:
        uart_printf("state:send req\n");
        HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
        HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
        RF_txQueue_Send((uint8_t*)&req_pkt, sizeof(req_pkt));

        g_slave_ctrl.start_wait = Get_SysTick_ms();
        g_slave_ctrl.state = SLAVE_PAIR_WAIT_RESP;
        break;

    case SLAVE_PAIR_WAIT_RESP:
        //检查是否有数据 (非阻塞检查)
        if (RF_rxQueue_Recv(&recv_resp_pkg, &len, NULL) == 1) {
            if (len == sizeof(pair_resp_pkt) && recv_resp_pkg->cmd == CMD_PAIR_RESP) {
                 uint32_t new_addr[5];
                 for(int i=0;i<5;i++) new_addr[i] = recv_resp_pkg->new_addr[i];

                 HAL_RF_SetTxAddress(&hrf, new_addr, 5);
                 HAL_RF_SetRxAddress(&hrf, 0, new_addr, 5);
                 
                 uart_printf("Slave: Got Resp! Switch Addr.\n");
                 g_slave_ctrl.state = SLAVE_PAIR_SEND_CONFIRM;
                 return; // 状态跃迁后立即退出，下一次循环处理新状态
            }
        }

        //检查超时
        if (Get_SysTick_ms() - g_slave_ctrl.start_wait > RESP_WAIT_TIMEOUT) {
            uart_printf("Wait Resp Timeout, Retry...\n");
            g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
        }
        break;

    case SLAVE_PAIR_SEND_CONFIRM:
        uart_printf("state:send confirm (%d)\n", g_slave_ctrl.confirm_retries + 1);

        //启动发送，发三包以提高成功率
        for(uint8_t i=0;i<3;i++)
            RF_txQueue_Send((uint8_t*)&slave_cfm_pkg, sizeof(slave_cfm_pkg));
                
        //记录快照，进入等待状态
        g_slave_ctrl.txds_snapshot = rf_int_count_txds; 
        g_slave_ctrl.start_wait = Get_SysTick_ms(); //复用 start_wait 做 TX 超时
        g_slave_ctrl.state = SLAVE_PAIR_WAIT_CONFIRM_TX_DONE;
        break;

    case SLAVE_PAIR_WAIT_CONFIRM_TX_DONE:
        if (rf_int_count_txds > g_slave_ctrl.txds_snapshot) {
            uart_printf("Slave: Confirm Sent! Switch to RX.\n");
                        
            //设置等待ACK的超时时间
            g_slave_ctrl.start_wait = Get_SysTick_ms(); 
            g_slave_ctrl.state = SLAVE_PAIR_WAIT_CONFIRM_ACK;
        }
        //检查发送超时
        else if (Get_SysTick_ms() - g_slave_ctrl.start_wait > 500) {
            uart_printf("Slave: Confirm TX Timeout/Error! Retry...\n");
            g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ; //回到发送REQ重新开始
        }
        break;

    case SLAVE_PAIR_WAIT_CONFIRM_ACK:
        HAL_RF_SetRxMode(&hrf); 
        uart_printf("state:wait confirm ack\n");
        //检查接收队列是否有数据
        if (RF_rxQueue_Recv(&recv_cfm_pkg, &len, NULL) == 1) {
            if (len == sizeof(pair_confirm_pkt) &&
                recv_cfm_pkg->cmd == CMD_PAIR_CONFIRM &&
                recv_cfm_pkg->magic_number == host_magic_number) {
                
                uart_printf("Slave: connect,now ping-pong\n");
                
                g_slave_ctrl.start_wait = Get_SysTick_ms();
                g_slave_ctrl.state = SLAVE_PAIR_VERIFY_PHASE;
                return;
            }
        }
        //检查超时
        if (Get_SysTick_ms() - g_slave_ctrl.start_wait > CONFIRM_WAIT_TIMEOUT) {
                uart_printf("Pairing Failed, Restarting...\n");
                g_slave_ctrl.confirm_retries = 0;
                g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
        }
        break;

    /* 监听host的ping */
    case SLAVE_PAIR_VERIFY_PHASE:
        if (RF_rxQueue_Recv(&recv_ping_pkt, &len, NULL) == 1 &&
            len == sizeof(pair_confirm_pkt) &&
            recv_ping_pkt->cmd == CMD_PAIR_VERIFY_PING &&
            recv_ping_pkt->magic_number == host_magic_number ) {
                                                            
                RF_txQueue_Send((uint8_t*)&pong_pkt, sizeof(pong_pkt));
                
                g_slave_ctrl.verify_cnt++;
                uart_printf("Slave: Verified %d/3\n", g_slave_ctrl.verify_cnt);

                if (g_slave_ctrl.verify_cnt >= 3) {
                    uart_printf("Slave: Pairing  SUCCESS!\n");
                    g_slave_ctrl.state = SLAVE_PAIR_DONE;
                }
        }
        
        if (Get_SysTick_ms() - g_slave_ctrl.start_wait > 2000) {
            uart_printf("Slave: Verify Timeout! Link Unstable.\n");
            g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
        }
            break;

    case SLAVE_PAIR_DONE:
        g_slave_ctrl.is_running = 0; //任务结束
        uart_printf("Slave: Pairing Process Success Completed.\n");
        printf_txrx_addr();
        break;

    }
}
    


// Host配对控制块
typedef struct {
    host_pair_state_t state;
    uint8_t is_running;
    
    uint32_t start_wait;       // 通用超时计时起点
    uint8_t resp_retries;      // 重发计数
    
    // 关键数据缓存 (必须静态保存，否则退出函数就丢了)
    pair_resp_pkt resp_pkt;    
    pair_confirm_pkt host_cfm_pkt;
    
    // 发送成功计数快照 (用于send_resp和Final Confirm 判断)
    uint32_t txds_snapshot;  
    
    uint8_t verify_cnt;       //配对验证计数，用于处理ping/pong阶段
    uint32_t last_ping_tick;  //上次收到ping的时间戳
} HostPairCtrl_t;

static HostPairCtrl_t g_host_ctrl;

void Host_Pairing_Start(void) {
    // 初始化控制块
    memset(&g_host_ctrl, 0, sizeof(g_host_ctrl));
    g_host_ctrl.state = HOST_PAIR_IDLE;
    g_host_ctrl.is_running = 1;
    g_host_ctrl.host_cfm_pkt.cmd = CMD_PAIR_CONFIRM;
    g_host_ctrl.host_cfm_pkt.magic_number = host_magic_number;

    // 初始化地址池 (只需做一次，建议系统启动时做，或者在这里做)
    ADDR_POOL_INIT_NORESERVED(&g_addr3);
    ADDR_POOL_INIT_NORESERVED(&g_addr4);

    // 初始化 RF
    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxMode(&hrf);
    
    uart_printf("Host: Start Pairing Task...\n");
}

void Host_Pairing_Stop(void) {
    if (g_host_ctrl.is_running) {
        g_host_ctrl.is_running = 0;
        g_host_ctrl.state = HOST_PAIR_IDLE;
        uart_printf("Host: Pairing Process Stopped.\n");
    }
}

void Host_Pairing_Task(void) {
    if (!g_host_ctrl.is_running) return;

    pair_req_pkt *recv_req = NULL;
    pair_confirm_pkt *recv_cfm = NULL;

    pair_confirm_pkt *recv_pong_pkt = NULL;
    pair_confirm_pkt  ping_pkt = {CMD_PAIR_VERIFY_PING, host_magic_number};

    uint8_t len;
    
    const uint8_t MAX_RESP_RETRIES = 3;
    const uint16_t CONFIRM_WAIT_TIMEOUT = 1000;
    const uint16_t FINAL_SEND_DURATION = 1000;

    //维持RF底层状态机
    RF_Service_Handler(&hrf);

    switch (g_host_ctrl.state) {
    case HOST_PAIR_IDLE:
        // Start 中已完成初始化，直接进等待状态
        g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
        uart_printf("Host: Waiting for Req...\n");
        break;

    /* 等待slave请求 */
    case HOST_PAIR_WAIT_REQ:  
        HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
        HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
        // 非阻塞检查接收队列
        if (RF_rxQueue_Recv(&recv_req, &len, NULL) == 1) {
            if (len == sizeof(pair_req_pkt) && recv_req->cmd == CMD_PAIR_REQ) {
                uart_printf("Host: Recv Req from %08X\n", recv_req->slave_id);
                
                // 准备响应包
                g_host_ctrl.resp_pkt.cmd = CMD_PAIR_RESP;
                g_host_ctrl.resp_pkt.new_addr[0] = ADDR_BASE_BYTE0;
                g_host_ctrl.resp_pkt.new_addr[1] = ADDR_BASE_BYTE1;
                g_host_ctrl.resp_pkt.new_addr[2] = ADDR_BASE_BYTE2;

                // 分配最后两位地址
                if(addrpool_alloc_addr_random(&g_addr3, &g_host_ctrl.resp_pkt.new_addr[3]) != 0 ||
                   addrpool_alloc_addr_random(&g_addr4, &g_host_ctrl.resp_pkt.new_addr[4]) != 0) {
                    uart_printf("Host: Addr Alloc Failed! Ignore Req.\n");
                    break; //分配失败
                }

                //准备进入发送阶段
                g_host_ctrl.resp_retries = 0;
                g_host_ctrl.state = HOST_PAIR_SEND_RESP;
            }
        }
        break;

    /* 发送响应包 */
   case HOST_PAIR_SEND_RESP:
        uart_printf("Host: Sending RESP (try %d)\n", g_host_ctrl.resp_retries + 1);
        
        HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
        HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
        //为了提高成功率，连续写三包
        for(uint8_t i=0; i<3; i++)
            RF_txQueue_Send((uint8_t*)&g_host_ctrl.resp_pkt, sizeof(g_host_ctrl.resp_pkt));
        
        //记录发送前的快照，并开始超时计时
        g_host_ctrl.txds_snapshot = rf_int_count_txds; 
        g_host_ctrl.start_wait = Get_SysTick_ms();
        
        //等待发送完成
        g_host_ctrl.state = HOST_PAIR_WAIT_RESP_TX_DONE;
        break;

    case HOST_PAIR_WAIT_RESP_TX_DONE:
        //检查tx完成计数器是否增加
        if (rf_int_count_txds > g_host_ctrl.txds_snapshot) {
            uart_printf("Host: RESP Sent! Switch Addr.\n");
            //剩下的几包响应帧清空，防止残留数据干扰下次发送
            RF_txQueue_Clear();
            g_host_ctrl.resp_retries++;

            //提取新地址
            uint32_t new_addr[5];
            for(int i=0; i<5; i++) new_addr[i] = g_host_ctrl.resp_pkt.new_addr[i];

            // 切换到地址监听slave的 confirm
            HAL_RF_SetTxAddress(&hrf, new_addr, 5);
            HAL_RF_SetRxAddress(&hrf, 0, new_addr, 5);
            HAL_RF_SetRxMode(&hrf);

            // 设置等待 Confirm 的超时
            g_host_ctrl.start_wait = Get_SysTick_ms();
            g_host_ctrl.state = HOST_PAIR_WAIT_CONFIRM;
        }
        //检查发送超时
        else if (Get_SysTick_ms() - g_host_ctrl.start_wait > 100) {
            uart_printf("Host: RESP TX Timeout/Error! return wait_req...\n");
            g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
        }
        break;

    case HOST_PAIR_WAIT_CONFIRM:
        //检查是否收到 Confirm
        if (RF_rxQueue_Recv(&recv_cfm, &len, NULL) == 1) {
            if (len == sizeof(pair_confirm_pkt) &&
                recv_cfm->cmd == CMD_PAIR_CONFIRM &&
                recv_cfm->magic_number == slave_magic_number) {
                
                uart_printf("Host: Recv Confirm! Sending Final Ack...\n");
                
                //记录当前发送完成计数，用于判断是否发送成功
                g_host_ctrl.txds_snapshot = rf_int_count_txds; 
                g_host_ctrl.start_wait = Get_SysTick_ms();
                g_host_ctrl.state = HOST_PAIR_SEND_FINAL_CONFIRM;
                return;
            }
        }

        //检查超时
        if (Get_SysTick_ms() - g_host_ctrl.start_wait > CONFIRM_WAIT_TIMEOUT) {
            if (g_host_ctrl.resp_retries < MAX_RESP_RETRIES) {
                uart_printf("Host: Confirm Timeout, Resend RESP...\n");
                g_host_ctrl.state = HOST_PAIR_SEND_RESP; //回去重发
            } else {
                uart_printf("Host: Max Retries Reached. Reset.\n");
                //恢复默认配置
                HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
                HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
                HAL_RF_SetRxMode(&hrf);
                g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
            }
        }
        break;

    case HOST_PAIR_SEND_FINAL_CONFIRM:  
        //写三包，尝试发送
        for(uint8_t i=0; i<3; i++)
            RF_txQueue_Send((uint8_t*)&g_host_ctrl.host_cfm_pkt, sizeof(g_host_ctrl.host_cfm_pkt));

            // 检查是否有新的发送完成中断
        if (rf_int_count_txds > g_host_ctrl.txds_snapshot) {
            uart_printf("Host: Confirm_Ack Sent Successfully!\n");
            //g_host_ctrl.is_running = 0;

            //进入ping-pong验证阶段
            g_host_ctrl.start_wait = Get_SysTick_ms();
            g_host_ctrl.state = HOST_PAIR_VERIFY_PHASE;
            for(uint8_t i=0; i<10; i++)
                RF_txQueue_Send((uint8_t*)&ping_pkt, sizeof(ping_pkt));
            
            return;
        }

        // 检查超时
        if (Get_SysTick_ms() - g_host_ctrl.start_wait > FINAL_SEND_DURATION) {
            uart_printf("Host: Confirm_ACK Send Failed/Timeout. Reset.\n");
            
            // 失败回退
            HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxMode(&hrf);
            g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
        }
        break;

    /* 主动ping slave，解析slave的pong包 */
    case HOST_PAIR_VERIFY_PHASE:
        
        if (RF_rxQueue_Recv(&recv_pong_pkt, &len, NULL) == 1 &&
            recv_pong_pkt->cmd == CMD_PAIR_VERIFY_PONG &&
            recv_pong_pkt->magic_number == slave_magic_number) {
                
                g_host_ctrl.verify_cnt++;
                uart_printf("Host: Verified %d/3\n", g_host_ctrl.verify_cnt);

                if (g_host_ctrl.verify_cnt >= 3) {
                    uart_printf("Host: ping-pong  SUCCESS!\n");
                    g_host_ctrl.state = HOST_PAIR_DONE;
                }
        }
        if (Get_SysTick_ms() - g_host_ctrl.start_wait > 2000) {
            uart_printf("Host: Verify Timeout! Link Unstable.\n");
            g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
        }
        break;
        
    case HOST_PAIR_DONE:
        g_host_ctrl.is_running = 0; //任务结束
        uart_printf("Host: Pairing Process Success Completed.\n");
        printf_txrx_addr();
        break;
    }
}
