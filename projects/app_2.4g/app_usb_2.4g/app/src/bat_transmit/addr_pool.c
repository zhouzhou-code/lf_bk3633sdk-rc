
#include <stdio.h>
#include <stdint.h>

#include "addr_pool.h"

/* 地址池管理模块 */
//实际通信地址由base地址+后缀ID组成，base地址固定，后缀ID(0~255)由地址池分配



// 地址池状态位图：0表示空闲，1表示已占用
// 256个比特需要32字节(256/8 = 32)
static uint8_t g_addr_usage_bitmap[ADDR_POOL_SIZE / 8] = {0};
//系统时间,用于随机数
static uint32_t (*get_systick_ms)(void) = NULL;

/**
 * @brief 标记ID为已使用，将ID在位图上的对应位:第(id/8)字节的第(id%8)位置1
 */
void addrpool_occupy(uint8_t id) {
    //将对应位:第(id/8)字节的第(id%8)位置1
    g_addr_usage_bitmap[id / 8] |= (1 << (id % 8));
}

/**
 * @brief 释放ID，将ID在位图上的对应位:第(id/8)字节的第(id%8)位置0
 */
void addrpool_free(uint8_t id) {
    g_addr_usage_bitmap[id / 8] &= ~(1 << (id % 8));
}

/**
 * @brief 顺序查找到第一个可用的 ID
 * @param[out] id 找到的可用 ID 存放位置
 * @return 0-成功找到可用 ID 并返回，-1-地址池已满
 */
int8_t addrpool_alloc_addr_first(uint8_t *out_id) {
    for (int i = 0; i < 32; i++) {
        // 如果整个字节都是 0xFF，说明这8个位全满了，直接跳过，增加查找效率
        if (g_addr_usage_bitmap[i] == 0xFF) continue;

        for (int j = 0; j < 8; j++) {
            if (!(g_addr_usage_bitmap[i] & (1 << j))) {
                //标记占用
                g_addr_usage_bitmap[i] |= (1 << j);
                *out_id = (uint8_t)(i * 8 + j);
                return 0;
            }
        }
    }
    return -1;
}

void addrpool_register_get_systick_ms(uint32_t (*get_systick_ms_func)(void)){
    get_systick_ms = get_systick_ms_func;
}


// 假设你有一些不想分配的保留ID（比如 0x00, 0xFF 或配对用的特殊ID）
static uint8_t Is_Reserved_ID(uint8_t id) {
    //if (id == 0x00 || id == 0xFF || id == 0xAA || id == 0x55) return 1;
    return 0;
}
/**
 * @brief 随机分配一个可用的 ID 并标记占用
 * @param out_id 输出参数，返回分配到的ID
 * @return 0: 成功, -1: 失败(池满了)
 */
int8_t addrpool_alloc_addr_random(uint8_t *out_id) {
    //获取一个随机起始点
    uint8_t start_index = (uint8_t)(get_systick_ms() & 0xFF); 

    //从随机点开始，尝试 256 次 (保证遍历整个池)
    for (int i = 0; i < 256; i++) {
        // 计算当前要检查的 ID (利用取模实现循环查找)
        uint8_t curr_id = (start_index + i) % 256;

        if (Is_Reserved_ID(curr_id)) {
            continue;
        }

        // 4. 检查位图状态
        uint8_t byte_idx = curr_id / 8;
        uint8_t bit_idx  = curr_id % 8;

        if (!(g_addr_usage_bitmap[byte_idx] & (1 << bit_idx))) {
            g_addr_usage_bitmap[byte_idx] |= (1 << bit_idx);
            
            //输出结果
            *out_id = curr_id;
            return 0; 
        }
    }

    return -1;
}



/**
 * @brief 调试打印：显示当前已分配了多少个地址
 */
void addrpool_print_summary(void) {
    uint16_t used_count = 0;
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 8; j++) {
            if (g_addr_usage_bitmap[i] & (1 << j)) used_count++;
        }
    }
    //uart_printf("AddrPool: %d / 256 slots used.\r\n", used_count);
}