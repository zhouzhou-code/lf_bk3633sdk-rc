#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint8_t *p_buffer;      /* 实际的内存池首地址 */
    uint16_t item_size;     /* 每一项数据的大小 (字节) */
    uint16_t item_count;    /* 队列项数 */
    volatile uint16_t in;    /* 写索引 (Write/Producer) */
    volatile uint16_t out;   /* 读索引 (Read/Consumer) */
} my_queue_t;


void queue_init(my_queue_t *queue, void *buffer, uint16_t buffer_size, uint16_t item_count, uint16_t item_size);

uint8_t queue_push(my_queue_t *queue, const void *p_src);
uint8_t queue_push_overwrite(my_queue_t *queue, const void *p_src);
uint8_t queue_pop(my_queue_t *queue, void *p_dest);
uint8_t queue_remove(my_queue_t *queue, uint16_t len);
uint8_t queue_peek(my_queue_t *queue, void *p_dest);

uint8_t queue_is_full(my_queue_t *queue);

#define queue_clear(q)          do { q->in = q->out = 0; } while(0)     //清空队列
#define queue_get_write_ptr(q)  (q->p_buffer + (q->in * q->item_size))  //获取写指针(初地址+写索引偏移)
#define queue_get_read_ptr(q)   (q->p_buffer + (q->out * q->item_size)) //获取读指针(初地址+读索引偏移)
#define queue_get_counts(q)     ((q->in >= q->out) ? (q->in - q->out) : (q->item_count - q->out + q->in)) //获取队列中数据项数
#define queue_is_empty(q)       (q->in == q->out ? 1 : 0)

#endif /* __UNI_FIFO_H__ */