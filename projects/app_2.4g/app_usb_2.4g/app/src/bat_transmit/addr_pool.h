#ifndef __ADDR_POOL_H__
#define __ADDR_POOL_H__


#include <stdint.h>

#define ADDR_POOL_SIZE      256   // 总容量 256 个
#define ADDR_POOL_START_ID  0x00  // 从 0x00 开始
#define ADDR_BASE_BYTE0     0x11  // 你的固定前缀
#define ADDR_BASE_BYTE1     0x22
#define ADDR_BASE_BYTE2     0x33
#define ADDR_BASE_BYTE3     0x44


void addrpool_register_get_systick_ms(uint32_t (*get_systick_ms_func)(void));
void addrpool_occupy(uint8_t id);
void addrpool_free(uint8_t id);
int8_t addrpool_alloc_addr_first(uint8_t *out_id) ;
int8_t addrpool_alloc_addr_random(uint8_t *out_id) ;
void addrpool_print_summary(void);


#endif // __ADDR_POOL_H__