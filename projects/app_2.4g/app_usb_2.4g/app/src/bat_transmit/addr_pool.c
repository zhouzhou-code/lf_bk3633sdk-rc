
#include <stdio.h>
#include <stdint.h>

#include "addr_pool.h"

// 地址池状态位图：0表示空闲，1表示已占用
// 256个比特需要32字节(256/8 = 32)
static uint8_t g_addr_usage_bitmap[ADDR_POOL_SIZE / 8] = {0};
//系统时间,用于随机数
static uint32_t (*get_systick_ms)(void) = NULL;

//初始化地址池，传入保留ID列表
void __addrpool_init_internal(SingleByteAddrPool_t *pool, uint8_t *list, uint16_t count)
{
    if (!pool||count>ADDR_POOL_SIZE) return;

    memset(pool, 0, sizeof(SingleByteAddrPool_t));

    if (list != NULL && count > 0) {
        for (uint16_t i = 0; i < count; i++) {
            uint8_t id = list[i];
            //将id对应的位在 reserved_id 数组中置 1
            pool->reserved_id[id / 8] |= (1 << (id % 8));
        }
    }
}


/**
 * @brief 标记ID为已使用，将ID在位图上的对应位:第(id/8)字节的第(id%8)位置1
 */
void addrpool_occupy(SingleByteAddrPool_t* pool, uint8_t id) {
    //将对应位:第(id/8)字节的第(id%8)位置1
    pool->bitmap[id / 8] |= (1 << (id % 8));
}

/**
 * @brief 释放ID，将ID在位图上的对应位:第(id/8)字节的第(id%8)位置0
 */
void addrpool_free(SingleByteAddrPool_t* pool, uint8_t id) {
    pool->bitmap[id / 8] &= ~(1 << (id % 8));
}

/**
 * @brief 顺序查找到第一个可用的 ID (同时避开已占用和保留的ID)
 * @param[out] out_id 找到的可用 ID 存放位置
 * @return 0: 成功, -1: 地址池已满
 */
int8_t addrpool_alloc_addr_first(SingleByteAddrPool_t* pool, uint8_t *out_id) {
    if (!pool || !out_id) return -1;

    for (int i = 0; i < 32; i++) {
        // 计算当前字节的“不可用掩码”
        // 不可用 = (已被占用) 或 (被保留)
        uint8_t unavailable_mask = pool->bitmap[i] | pool->reserved_id[i];

        // 如果该字节所有位都不可用 既没空位，或者是保留位，直接跳过
        if (unavailable_mask == 0xFF) continue;

        for (int j = 0; j < 8; j++) {
            // 检查特定位是否“可用”
            // 只有当 unavailable_mask 的第 j 位为 0 时，才表示既没被占，也没保留
            if (!(unavailable_mask & (1 << j))) {
                
                // 找到可用 ID，标记占用
                pool->bitmap[i] |= (1 << j);
                *out_id = (uint8_t)(i * 8 + j);
                return 0;
            }
        }
    }
    return -1; // 找遍了所有字节，满
}

void addrpool_register_get_systick_ms(uint32_t (*get_systick_ms_func)(void)){
    get_systick_ms = get_systick_ms_func;
}

/**
 * @brief 随机分配一个可用的 ID 并标记占用
 * @param out_id 输出参数，返回分配到的ID
 * @return 0: 成功, -1: 失败(池满了)
 */
int8_t addrpool_alloc_addr_random(SingleByteAddrPool_t* pool,uint8_t *out_id) {
    //获取一个随机起始点
    uint8_t start_index = (uint8_t)(get_systick_ms() & 0xFF); 

    //从随机点开始，尝试 256 次 (保证遍历整个池)
    for (int i = 0; i < 256; i++) {
        // 计算当前要检查的 ID
        uint8_t curr_id = (start_index + i) % 256;
        uint8_t byte_idx = curr_id / 8;
        uint8_t bit_idx  = curr_id % 8;

        // 检查占用位图
        uint8_t occupied = pool->bitmap[byte_idx] & (1 << bit_idx);
        // 检查保留位图
        uint8_t reserved = pool->reserved_id[byte_idx] & (1 << bit_idx);

        //只有当未被占用且非保留时，才分配
        if (!occupied && !reserved) {
            pool->bitmap[byte_idx] |= (1 << bit_idx); // 标记占用
            *out_id = curr_id;
            
            return 0; // 成功
        }
    }

    return -1;
}



/**
 * @brief 调试打印：显示当前已分配了多少个地址
 */
// void addrpool_print_summary(void) {
//     uint16_t used_count = 0;
//     for (int i = 0; i < 32; i++) {
//         for (int j = 0; j < 8; j++) {
//             if (pool->bitmap[i] & (1 << j)) used_count++;
//         }
//     }
//     //uart_printf("AddrPool: %d / 256 slots used.\r\n", used_count);
// }