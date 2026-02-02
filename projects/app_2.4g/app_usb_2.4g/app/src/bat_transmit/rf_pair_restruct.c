#include "driver_bk24.h"
#include "hal_drv_rf.h"
#include "rf_handler.h" 
#include "rf_pair_restruct.h" 
#include "user_config.h" 
#include "app.h"
#include "bk3633_reglist.h"
#include "driver_timer.h"
#include <string.h>
#include "uart.h"
#include "addr_pool.h"

// 全局标志
uint8_t slave_pair_success_flag = 0;
uint8_t host_pair_success_flag = 0;

static const uint32_t PAIR_ADDR_DEFAULT[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

static uint8_t slave_magic_number = 0x5A; 
static uint8_t host_magic_number  = 0xA5;

SingleByteAddrPool_t g_addr3;
SingleByteAddrPool_t g_addr4;

// 调试打印函数
static void printf_txrx_addr(void)
{
    uart_printf("RX_ADDR: ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_RX_ADDR_P0_0)[i]);
    uart_printf(" | TX_ADDR: ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_TX_ADDR_0)[i]);
    uart_printf("\r\n");
}

// 全局超时检查 (防止死锁)
#define GLOBAL_TIMEOUT_MS 10000
static uint8_t Check_Global_Timeout(uint32_t start_tick) {
    if (Get_SysTick_ms() - start_tick > GLOBAL_TIMEOUT_MS) {
        return 1;
    }
    return 0;
}

static SlavePairCtrl_t g_slave_ctrl = {SLAVE_PAIR_IDLE, 0, 0, 0, 0};

void Slave_Pairing_Start(void) {
    memset(&g_slave_ctrl, 0, sizeof(g_slave_ctrl));
    g_slave_ctrl.state = SLAVE_PAIR_IDLE;
    g_slave_ctrl.is_running = 1;
    g_slave_ctrl.global_start_tick = Get_SysTick_ms();
    
    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetTxMode(&hrf);
    uart_printf("Slave: Start Pairing Task...\n");
}

void Slave_Pairing_Stop(void) {
    if (g_slave_ctrl.is_running) {
        g_slave_ctrl.is_running = 0;
        g_slave_ctrl.state = SLAVE_PAIR_IDLE;
        uart_printf("Slave: Pairing Stopped.\n");
    }
}

void Slave_Pairing_Task(void) {
    if (g_slave_ctrl.is_running == 0) return;

    pair_req_pkt req_pkt = {CMD_PAIR_REQ, 0x12345678};
    pair_resp_pkt *recv_resp_pkg = NULL;
    pair_verify_pkt pong_pkt = {CMD_PAIR_VERIFY_PONG, slave_magic_number};
    pair_verify_pkt *recv_ping_pkt = NULL;
    uint8_t len;
    
    const uint16_t RESP_WAIT_TIMEOUT = 200;  // 
    const uint16_t VERIFY_TIMEOUT = 2000;   //

    // 维持RF底层
    RF_Service_Handler(&hrf);

    // 全局超时保护
    if (Check_Global_Timeout(g_slave_ctrl.global_start_tick)) {
        uart_printf("Slave: Global Timeout! Reset.\n");
        Slave_Pairing_Start(); // 重置状态
        return;
    }

    switch (g_slave_ctrl.state) {
        case SLAVE_PAIR_IDLE:
            g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
            break;

        case SLAVE_PAIR_SEND_REQ:
            uart_printf("Slave: Send REQ\n");
            // 确保地址正确
            HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
            RF_txQueue_Send((uint8_t*)&req_pkt, sizeof(req_pkt));

            g_slave_ctrl.start_wait = Get_SysTick_ms();
            g_slave_ctrl.state = SLAVE_PAIR_WAIT_RESP;
            break;

        case SLAVE_PAIR_WAIT_RESP:
            if (RF_rxQueue_Recv(&recv_resp_pkg, &len, NULL) == 1) {
                if (len == sizeof(pair_resp_pkt) && recv_resp_pkg->cmd == CMD_PAIR_RESP) {
                    
                    uint32_t new_addr[5];
                    for(int i=0; i<5; i++) new_addr[i] = recv_resp_pkg->new_addr[i];

                    uart_printf("Slave: Got RESP! Switch to New Addr.\n");

                    // 立即切换地址
                    HAL_RF_SetTxAddress(&hrf, new_addr, 5);
                    HAL_RF_SetRxAddress(&hrf, 0, new_addr, 5);

                    //进入验证
                    g_slave_ctrl.verify_cnt = 0;
                    g_slave_ctrl.start_wait = Get_SysTick_ms();
                    g_slave_ctrl.state = SLAVE_PAIR_VERIFY_PHASE;
                    return; 
                }
            }

            //超时快速重发
            if (Get_SysTick_ms() - g_slave_ctrl.start_wait > RESP_WAIT_TIMEOUT) {
                g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
            }
            break;

        case SLAVE_PAIR_VERIFY_PHASE:
            //监听Ping
            if (RF_rxQueue_Recv(&recv_ping_pkt, &len, NULL) == 1) {
                if (len == sizeof(pair_verify_pkt) &&
                    recv_ping_pkt->cmd == CMD_PAIR_VERIFY_PING &&
                    recv_ping_pkt->magic_number == host_magic_number) {
                    
                    // 收到 Ping，回复 Pong
                    RF_txQueue_Send((uint8_t*)&pong_pkt, sizeof(pong_pkt));
                    
                    g_slave_ctrl.verify_cnt++;
                    uart_printf("Slave: Verified %d/5\n", g_slave_ctrl.verify_cnt);

                    if (g_slave_ctrl.verify_cnt >= 5) {
                        uart_printf("Slave: Pairing SUCCESS!\n");
                        printf_txrx_addr();
                        g_slave_ctrl.state = SLAVE_PAIR_DONE;
                    }
                }
            }
            
            //验证阶段超时 说明 Host 没过来，或地址错了
            if (Get_SysTick_ms() - g_slave_ctrl.start_wait > VERIFY_TIMEOUT) {
                uart_printf("Slave: Verify Timeout! Revert.\n");
                g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
            }
            break;

        case SLAVE_PAIR_DONE:
            g_slave_ctrl.is_running = 0;
            uart_printf("Slave: Pairing Process Success Completed.\n");
            printf_txrx_addr();
            break;
    }
}



static HostPairCtrl_t g_host_ctrl;

void Host_Pairing_Start(void) {
    memset(&g_host_ctrl, 0, sizeof(g_host_ctrl));
    g_host_ctrl.state = HOST_PAIR_IDLE;
    g_host_ctrl.is_running = 1;
    g_host_ctrl.global_start_tick = Get_SysTick_ms();

    // 初始化地址池
    ADDR_POOL_INIT_NORESERVED(&g_addr3);
    ADDR_POOL_INIT_NORESERVED(&g_addr4);

    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxMode(&hrf);
    
    uart_printf("Host: Start Pairing Task...\n");
}

void Host_Pairing_Stop(void) {
    if (g_host_ctrl.is_running) {
        g_host_ctrl.is_running = 0;
        g_host_ctrl.state = HOST_PAIR_IDLE;
        uart_printf("Host: Pairing Stopped.\n");
    }
}

void Host_Pairing_Task(void) {
    if (g_host_ctrl.is_running == 0) return;

    pair_req_pkt *recv_req = NULL;
    pair_verify_pkt *recv_pong_pkt = NULL;
    pair_verify_pkt ping_pkt = {CMD_PAIR_VERIFY_PING, host_magic_number};
    uint8_t len;
    
    RF_Service_Handler(&hrf);

    // 全局超时保护
    if (Check_Global_Timeout(g_host_ctrl.global_start_tick)) {
        uart_printf("Host: Global Timeout! Reset.\n");
        Host_Pairing_Start();
        return;
    }

    switch (g_host_ctrl.state) {
        case HOST_PAIR_IDLE:
            g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
            uart_printf("Host: Waiting for Req...\n");
            break;

        case HOST_PAIR_WAIT_REQ:
            HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);

            if (RF_rxQueue_Recv(&recv_req, &len, NULL) == 1) {
                if (len == sizeof(pair_req_pkt) && recv_req->cmd == CMD_PAIR_REQ) {
                    uart_printf("Host: Recv Req from %08X\n", recv_req->slave_id);
                    
                    //准备 RESP 包
                    g_host_ctrl.resp_pkt.cmd = CMD_PAIR_RESP;
                    g_host_ctrl.resp_pkt.new_addr[0] = ADDR_BASE_BYTE0;
                    g_host_ctrl.resp_pkt.new_addr[1] = ADDR_BASE_BYTE1;
                    g_host_ctrl.resp_pkt.new_addr[2] = ADDR_BASE_BYTE2;

                    if(addrpool_alloc_addr_random(&g_addr3, &g_host_ctrl.resp_pkt.new_addr[3]) != 0 ||
                       addrpool_alloc_addr_random(&g_addr4, &g_host_ctrl.resp_pkt.new_addr[4]) != 0) {
                        uart_printf("Host: Addr Pool Full!\n");
                        break; 
                    }

                    //准备连发
                    g_host_ctrl.state = HOST_PAIR_SEND_RESP;
                }
            }
            break;

        case HOST_PAIR_SEND_RESP:
            uart_printf("Host: Burst Sending RESP (Blind Switch)...\n");
            
            HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
            
            // 连发5包
            RF_txQueue_Clear();
            for(int i=0; i<5; i++) 
                RF_txQueue_Send((uint8_t*)&g_host_ctrl.resp_pkt, sizeof(pair_resp_pkt));
            
            // 记录基准
            g_host_ctrl.txds_snapshot = rf_int_count_txds; 
            g_host_ctrl.start_wait = Get_SysTick_ms();
            g_host_ctrl.state = HOST_PAIR_WAIT_TX_DONE;
            break;

        case HOST_PAIR_WAIT_TX_DONE:
            if (rf_int_count_txds >= g_host_ctrl.txds_snapshot) {
                uart_printf("Host: Burst Done. Switch Addr.\n");
                RF_txQueue_Clear();
                //切换新地址
                uint32_t new_addr[5];
                for(int i=0; i<5; i++) new_addr[i] = g_host_ctrl.resp_pkt.new_addr[i];

                HAL_RF_SetTxAddress(&hrf, new_addr, 5);
                HAL_RF_SetRxAddress(&hrf, 0, new_addr, 5);

                //进入验证
                g_host_ctrl.verify_cnt = 0;
                g_host_ctrl.last_ping_tick = 0; 
                g_host_ctrl.start_wait = Get_SysTick_ms();
                g_host_ctrl.state = HOST_PAIR_VERIFY_PHASE;
                // //发送20个PING包提高成功率
                // for(int i=0;i<20;i++)
                //     RF_txQueue_Send((uint8_t*)&ping_pkt, sizeof(ping_pkt));
            
            }
            else if (Get_SysTick_ms() - g_host_ctrl.start_wait > 50) {
                uart_printf("Host: TX Timeout! Skip.\n");
                g_host_ctrl.state = HOST_PAIR_WAIT_REQ; // 失败回退
            }
            break;

        case HOST_PAIR_VERIFY_PHASE:

        if (Get_SysTick_ms() - g_host_ctrl.last_ping_tick > 50) {
            RF_txQueue_Send((uint8_t*)&ping_pkt, sizeof(ping_pkt));
            g_host_ctrl.last_ping_tick = Get_SysTick_ms();
            // 发完这一个包后，RF_Service_Handler 会自动把它发出去
        }
        //接收PONG
        if (RF_rxQueue_Recv(&recv_pong_pkt, &len, NULL) == 1) {
            if (recv_pong_pkt->cmd == CMD_PAIR_VERIFY_PONG &&
                recv_pong_pkt->magic_number == slave_magic_number) {
                
                g_host_ctrl.verify_cnt++;
                uart_printf("Host: Verified %d/10\n", g_host_ctrl.verify_cnt);

                if (g_host_ctrl.verify_cnt >= 5) {
                    uart_printf("Host: Pairing SUCCESS!\n");
                    printf_txrx_addr();
                    g_host_ctrl.state = HOST_PAIR_DONE;
                    return;
                }
            }
        }

            // 如果切过来 2000ms 还没收到 PONG，说明 Slave 没跟过来
            if (Get_SysTick_ms() - g_host_ctrl.start_wait > 2000) {
                uart_printf("Host: Verify Timeout! Slave Lost. Revert.\n");
                
                RF_txQueue_Clear();
                g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
            }
            break;

        case HOST_PAIR_DONE:
            g_host_ctrl.is_running = 0;
            uart_printf("Host: Pairing Process Success Completed.\n");
            printf_txrx_addr();
            break;
    }
}