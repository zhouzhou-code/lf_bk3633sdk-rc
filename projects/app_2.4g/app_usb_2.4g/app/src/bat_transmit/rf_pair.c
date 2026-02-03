#include "driver_bk24.h"
#include "hal_drv_rf.h"
#include "rf_handler.h" 
#include "rf_pair.h" 
#include "user_config.h" 
#include "app.h"
#include "bk3633_reglist.h"
#include "driver_timer.h"
#include <string.h>
#include "uart.h"
#include "addr_pool.h"
#include "flash.h"
#include "bk3633_reglist.h"

#include "co_error.h"      // error definition
#include "rwip.h"
#include "ll.h"
#include "wdt.h"
#include "rwprf_config.h"






static const uint8_t PAIR_ADDR_DEFAULT[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

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

// 调试打印函数
static void printf_txrx_addr(void)
{
    uart_printf("RX_ADDR: ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_RX_ADDR_P0_0)[i]);
    uart_printf(" | TX_ADDR: ");
    for(int i=0;i<5;i++) uart_printf("%02X ", (volatile uint32_t*)(&TRX_TX_ADDR_0)[i]);
    uart_printf("\r\n");
}


// #define FLASH_OPCODE_PP      12   
// #define FLASH_OPCODE_SE      13  
// #define BIT_ADDRESS_SW       0
// #define BIT_OP_TYPE_SW       24
// #define BIT_OP_SW            29


// static void Flash_Erase_Force(uint32_t address)
// {
//     // A. 强制解锁 (关键！)
//     flash_wp_none();

//     GLOBAL_INT_DISABLE(); 

//     // B. 等待空闲
//     while(REG_FLASH_OPERATE_SW & 0x80000000);

//     // C. 发送擦除命令
//     REG_FLASH_OPERATE_SW = (  (address << BIT_ADDRESS_SW)
//                             | (FLASH_OPCODE_SE << BIT_OP_TYPE_SW)
//                             | (0x1 << BIT_OP_SW));

//     // D. 等待完成
//     while(REG_FLASH_OPERATE_SW & 0x80000000);

//     // E. 恢复保护 (安全起见)
//     extern void flash_wp_all(void);
//     flash_wp_all();

//     GLOBAL_INT_RESTORE(); 
// }

// //原厂函数写flash强制保护，无法写入用户区，故自定义写函数
// static void Flash_Write_User_Data(uint32_t address, uint8_t *data, uint32_t len)
// {
//     uint32_t i;
//     uint32_t addr = address & (~0x1F); 
//     uint32_t buf[8];                   
//     uint8_t *pb = (uint8_t *)&buf[0];

//     if (len == 0) return;

//     flash_wp_none(); 

//     GLOBAL_INT_DISABLE(); 

//     while(len) 
//     {
//         // 读取旧数据以处理非对齐写入 (Read-Modify-Write)
//         if((address & 0x1F) || (len < 32)) {
//             flash_read_data(pb, addr, 32);
//         } else {
//             memset(buf, 0xFF, 32); 
//         }

//         //填充新数据
//         for(i = (address & 0x1F); i < 32; i++) {
//             if(len) {
//                 pb[i] = *data++;
//                 address++;
//                 len--;
//             }
//         }

//         // 等待Flash空闲
//         while(REG_FLASH_OPERATE_SW & 0x80000000); 

//         // 填入硬件 FIFO
//         for (i=0; i<8; i++) REG_FLASH_DATA_SW_FLASH = buf[i]; 
//         // 发送 Page Program (PP) 命令
//         REG_FLASH_OPERATE_SW = (  (addr << BIT_ADDRESS_SW)
//                                 | (FLASH_OPCODE_PP << BIT_OP_TYPE_SW)
//                                 | (0x1 << BIT_OP_SW));
        
//         // 等待写入完成
//         while(REG_FLASH_OPERATE_SW & 0x80000000); 
//         addr += 32; 
//     }

//     //写完后立刻恢复默认保护，确保代码区安全
//     flash_wp_all(); 
//     GLOBAL_INT_RESTORE(); 
// }

#define OP_IDX_PP      12   // Page Program 索引
#define OP_IDX_SE      13   // Sector Erase 索引

#define BIT_ADDRESS_SW       0
#define BIT_OP_TYPE_SW       24
#define BIT_OP_SW            29


static void Flash_Force_Unlock(void)
{
    flash_write_sr(1, 0x80);
}

static void Flash_Erase_Force(uint32_t address)
{
    Flash_Force_Unlock();
    GLOBAL_INT_DISABLE(); 
    while(REG_FLASH_OPERATE_SW & 0x80000000);

    // 3. 发送擦除命令
    REG_FLASH_OPERATE_SW = (  (address << BIT_ADDRESS_SW)
                            | (OP_IDX_SE << BIT_OP_TYPE_SW)
                            | (0x1 << BIT_OP_SW));
    // 等待完成
    while(REG_FLASH_OPERATE_SW & 0x80000000);

    // 恢复保护
    flash_wp_all();

    GLOBAL_INT_RESTORE(); 
}

static void Flash_Write_User_Data(uint32_t address, uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t addr = address & (~0x1F); 
    uint32_t buf[8];                   
    uint8_t *pb = (uint8_t *)&buf[0];

    if (len == 0) return;

    // 1. 强制解锁
    Flash_Force_Unlock();

    GLOBAL_INT_DISABLE(); 

    while(len) 
    {
        // 准备数据 (Read-Modify-Write 保持不变)
        if((address & 0x1F) || (len < 32)) {
            flash_read_data(pb, addr, 32);
        } else {
            memset(buf, 0xFF, 32); 
        }

        for(i = (address & 0x1F); i < 32; i++) {
            if(len) {
                pb[i] = *data++;
                address++;
                len--;
            }
        }

        // 等待空闲
        while(REG_FLASH_OPERATE_SW & 0x80000000); 

        // 填入 FIFO
        for (i=0; i<8; i++) REG_FLASH_DATA_SW_FLASH = buf[i]; 

        // 发送写入命令 (使用索引 12)
        // 硬件自动发送 WREN
        REG_FLASH_OPERATE_SW = (  (addr << BIT_ADDRESS_SW)
                                | (OP_IDX_PP << BIT_OP_TYPE_SW)
                                | (0x1 << BIT_OP_SW));
        
        while(REG_FLASH_OPERATE_SW & 0x80000000); 

        addr += 32; 
    }

    // 2. 恢复保护
    flash_wp_all(); 

    GLOBAL_INT_RESTORE(); 
}


static uint8_t calc_checksum(PairInfo_t *info) {
    uint8_t sum = 0;
    for(int i=0; i<5; i++) sum += info->pair_addr[i];
    sum += info->padding[0];
    sum += info->padding[1];
    return sum;
}

void Save_Pair_Info(uint8_t *addr) {
    PairInfo_t info;
    
    // 准备数据
    info.magic = FLASH_MAGIC_NUM;
    memcpy(info.pair_addr, addr, 5);
    info.padding[0] = 0xFF; 
    info.padding[1] = 0xFF;
    info.checksum = calc_checksum(&info);
    
    uart_printf("[Storage] Saving to 0x%X: %02X %02X %02X %02X %02X\r\n", 
                FLASH_PAIR_ADDR, addr[0], addr[1], addr[2], addr[3], addr[4]);

    Flash_Erase_Force(FLASH_PAIR_ADDR);
    Flash_Write_User_Data(FLASH_PAIR_ADDR, (uint8_t*)&info, sizeof(PairInfo_t));
}

void Load_Pair_Info(void) {
    PairInfo_t info;

    flash_read(0, FLASH_PAIR_ADDR, sizeof(PairInfo_t), (uint8_t*)&info, NULL);
    
    //校验魔数和校验和
    if (info.magic == FLASH_MAGIC_NUM && calc_checksum(&info) == info.checksum) {
        uart_printf("[Storage] Load Success! Addr: %02X %02X %02X %02X %02X\r\n",
                info.pair_addr[0], info.pair_addr[1], info.pair_addr[2], 
                info.pair_addr[3], info.pair_addr[4]);
                
        //应用地址到RF
        HAL_RF_SetTxAddress(&hrf, info.pair_addr, 5);
        HAL_RF_SetRxAddress(&hrf, 0, info.pair_addr, 5);
    } else {
        uart_printf("[Storage] No valid info. Using Default.\r\n");
        uart_printf("Magic: 0x%08X, Checksum: 0x%02X\r\n", info.magic, info.checksum);
        
        uart_printf("addr=\r\n");
        for(int i=0;i<sizeof(PairInfo_t);i++){
            uart_printf("%02X ", ((uint8_t*)&info)[i]);
        }
    }
}

void Clear_Pair_Info(void) {
    // 擦除也需要解锁，保险起见先解锁
    extern void flash_wp_none(void); 
    flash_wp_none();
    
    flash_erase(0, FLASH_PAIR_ADDR, 0x1000, NULL);
    
    extern void flash_wp_all(void);
    flash_wp_all();
    
    uart_printf("[Storage] Cleared.\r\n");
}

static SlavePairCtrl_t g_slave_ctrl = {
    .state = SLAVE_PAIR_IDLE, 
    .is_running = 0,
    .start_wait = 0,
    .global_start_tick = 0,
    .verify_cnt = 0,
    .verify_target_count = 5,
    .reach_goal_flag = 0,
    .last_ping_recv_timestamp = 0
};

void Slave_Pairing_Start(void) {
    memset(&g_slave_ctrl, 0, sizeof(g_slave_ctrl));
    g_slave_ctrl.state = SLAVE_PAIR_IDLE;
    g_slave_ctrl.is_running = 1;
    g_slave_ctrl.global_start_tick = Get_SysTick_ms();
    g_slave_ctrl.verify_target_count = 5;
    
    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetTxMode(&hrf);
    printf_all_registers();
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
    
    const uint16_t RESP_WAIT_TIMEOUT = 100;  //这个时间可以短
    const uint16_t VERIFY_TIMEOUT = 5000;    //5s，这个时间不能太短

    // 维持RF底层
    RF_Service_Handler(&hrf);

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
            //uart_printf("Slave: Wait RESP:%d\n", Get_SysTick_ms());
            if (RF_rxQueue_Recv(&recv_resp_pkg, &len, NULL) == 1) {
                if (len == sizeof(pair_resp_pkt) && recv_resp_pkg->cmd == CMD_PAIR_RESP) {
                    
                    uart_printf("Slave: Got RESP! Switch to New Addr.\n");

                    memcpy(&g_slave_ctrl.resp_pkt, recv_resp_pkg, sizeof(pair_resp_pkt));

                    // 立即切换地址
                    RF_rxQueue_clear();
                    HAL_RF_SetTxAddress(&hrf, g_slave_ctrl.resp_pkt.new_addr, 5);
                    HAL_RF_SetRxAddress(&hrf, 0, g_slave_ctrl.resp_pkt.new_addr, 5);

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
                    
                    // 重置超时计时和最后一次收到ping时间
                    g_slave_ctrl.start_wait = Get_SysTick_ms();
                    g_slave_ctrl.last_ping_recv_timestamp = Get_SysTick_ms();
                    // 收到 Ping，回复 Pong
                    RF_txQueue_Send((uint8_t*)&pong_pkt, sizeof(pong_pkt));
                    
                    g_slave_ctrl.verify_cnt++;
                    uart_printf("Slave: Verified %d/%d\n", g_slave_ctrl.verify_cnt, g_slave_ctrl.verify_target_count);

                    if (g_slave_ctrl.verify_cnt >= g_slave_ctrl.verify_target_count) {
                        // 标记,host端达标了,但要陪Host跑到终点
                        g_slave_ctrl.reach_goal_flag = 1; 
                        uart_printf("Slave: ping pong reached!\n");
                    }
                }
            }
            //A：已经成功了,且Host很久(比如3秒)没ping->说明Host也好了
            if (g_slave_ctrl.reach_goal_flag == 1) {
                if (Get_SysTick_ms() - g_slave_ctrl.last_ping_recv_timestamp > 3000) {
                    uart_printf("Slave: Host silent, Pairing Process DONE.\n");
                    g_slave_ctrl.state = SLAVE_PAIR_DONE;
                }
            }
            //B：还没成功，但是总超时了 -> 说明配对失败,host没在这个地址发包
            else {
                if (Get_SysTick_ms() - g_slave_ctrl.start_wait > VERIFY_TIMEOUT) {
                    uart_printf("Slave: Verify Timeout! Failed.\n");
                    g_slave_ctrl.state = SLAVE_PAIR_SEND_REQ;
                }
            }

            break;

        case SLAVE_PAIR_DONE:
            g_slave_ctrl.is_running = 0;
            uart_printf("Slave: Pairing Process Success Completed.\n");
            printf_txrx_addr();
            Save_Pair_Info(g_slave_ctrl.resp_pkt.new_addr);
            Load_Pair_Info();
            break;
    }
}



static HostPairCtrl_t g_host_ctrl={
    .state = HOST_PAIR_IDLE, 
    .is_running = 0,
    .start_wait = 0,
    .global_start_tick = 0,
    .last_ping_tick = 0,
    .verify_cnt = 0,
    .verify_target_count = 10
};

void Host_Pairing_Start(void) {
    memset(&g_host_ctrl, 0, sizeof(g_host_ctrl));
    g_host_ctrl.state = HOST_PAIR_IDLE;
    g_host_ctrl.is_running = 1;
    g_host_ctrl.global_start_tick = Get_SysTick_ms();
    g_host_ctrl.verify_target_count = 10;
    // 初始化地址池
    ADDR_POOL_INIT_NORESERVED(&g_addr3);
    ADDR_POOL_INIT_NORESERVED(&g_addr4);

    HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
    HAL_RF_SetRxMode(&hrf);
    printf_all_registers();
    
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
                    //准备RESP包
                    g_host_ctrl.resp_pkt.cmd = CMD_PAIR_RESP;
                    g_host_ctrl.resp_pkt.new_addr[0] = ADDR_BASE_BYTE0;
                    g_host_ctrl.resp_pkt.new_addr[1] = ADDR_BASE_BYTE1;
                    g_host_ctrl.resp_pkt.new_addr[2] = ADDR_BASE_BYTE2;

                    if(addrpool_alloc_addr_random(&g_addr3, &g_host_ctrl.resp_pkt.new_addr[3]) != 0 ||
                       addrpool_alloc_addr_random(&g_addr4, &g_host_ctrl.resp_pkt.new_addr[4]) != 0) {
                        uart_printf("Host: Addr Pool Full!\n");
                        break; 
                    }
                    uart_printf("Host: New Addr: %02X %02X %02X %02X %02X\n",
                                g_host_ctrl.resp_pkt.new_addr[0],
                                g_host_ctrl.resp_pkt.new_addr[1],
                                g_host_ctrl.resp_pkt.new_addr[2],
                                g_host_ctrl.resp_pkt.new_addr[3],
                                g_host_ctrl.resp_pkt.new_addr[4]);

                    //准备连发
                    g_host_ctrl.state = HOST_PAIR_SEND_RESP;
                }
            }
            break;

        case HOST_PAIR_SEND_RESP:
            uart_printf("Host: Burst Sending RESP (Blind Switch)...\n");
            
            HAL_RF_SetTxAddress(&hrf, PAIR_ADDR_DEFAULT, 5);
            HAL_RF_SetRxAddress(&hrf, 0, PAIR_ADDR_DEFAULT, 5);
            
            //连放5包到发送队列
            RF_txQueue_Clear();
            RF_rxQueue_clear();
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
                HAL_RF_SetTxAddress(&hrf, g_host_ctrl.resp_pkt.new_addr, 5);
                HAL_RF_SetRxAddress(&hrf, 0, g_host_ctrl.resp_pkt.new_addr, 5);

                //进入验证
                g_host_ctrl.verify_cnt = 0;
                g_host_ctrl.last_ping_tick = 0; 
                g_host_ctrl.start_wait = Get_SysTick_ms();
                g_host_ctrl.state = HOST_PAIR_VERIFY_PHASE;
                uart_printf("Host: goto verify phase.\n");
            
            }
            else if (Get_SysTick_ms() - g_host_ctrl.start_wait > 50) {
                uart_printf("Host: TX Timeout! Skip.\n");
                g_host_ctrl.state = HOST_PAIR_WAIT_REQ; // 失败回退
            }
            break;

        case HOST_PAIR_VERIFY_PHASE:
        //定时PING 50ms一次
        if (Get_SysTick_ms() - g_host_ctrl.last_ping_tick > 50) {
            RF_txQueue_Send((uint8_t*)&ping_pkt, sizeof(ping_pkt));
            g_host_ctrl.last_ping_tick = Get_SysTick_ms();
            uart_printf("Host: Sent PING,time:%d\n", g_host_ctrl.last_ping_tick);
        }
        //接收PONG
        if (RF_rxQueue_Recv(&recv_pong_pkt, &len, NULL) == 1) {
            if (recv_pong_pkt->cmd == CMD_PAIR_VERIFY_PONG &&
                recv_pong_pkt->magic_number == slave_magic_number) {
                
                //收到 Pong，重置超时计时
                g_host_ctrl.start_wait = Get_SysTick_ms();

                g_host_ctrl.verify_cnt++;
                uart_printf("Host: Verified %d/%d\n", g_host_ctrl.verify_cnt, g_host_ctrl.verify_target_count);

                if (g_host_ctrl.verify_cnt >= g_host_ctrl.verify_target_count) {
                    uart_printf("Host: Pairing SUCCESS!\n");
                    printf_txrx_addr();
                    g_host_ctrl.state = HOST_PAIR_DONE;
                    return;
                }
            }
        }

        // 如果切过来 2000ms 还没收到 PONG，说明Slave没跟过来，回退
        if (Get_SysTick_ms() - g_host_ctrl.start_wait > 20000) {
            uart_printf("Host: Verify Timeout! Slave Lost. Revert.\n");
            RF_txQueue_Clear();
            g_host_ctrl.state = HOST_PAIR_WAIT_REQ;
        }
        break;

        case HOST_PAIR_DONE:
            g_host_ctrl.is_running = 0;
            uart_printf("Host: Pairing Process Success Completed.\n");
            printf_txrx_addr();
            //保存到flash
            Save_Pair_Info(g_host_ctrl.resp_pkt.new_addr);
            Load_Pair_Info(); 
            break;
    }
}