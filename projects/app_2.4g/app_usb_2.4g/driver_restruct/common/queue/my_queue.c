#include "my_queue.h"
#include "user_config.h"

#ifndef MIN(a,b)
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif 

/**
 * @brief  初始化队列
 * @param  queue: 队列控制块指针
 * @param  buffer: 用户分配的实际内存地址 (大小 >= item_count * item_size)
 * @param  item_count: 队列项数 (注意: 实际可用容量为 item_count - 1)
 * @param  item_size: 单个数据项的大小(字节)
 */
void queue_init(my_queue_t *queue, void *buffer, uint16_t buffer_size, uint16_t item_count, uint16_t item_size)
{
    if ((queue == NULL) || (buffer == NULL) || (item_count == 0) || (item_size == 0)) {
        return;
    }
    if(buffer_size < (item_count * item_size)) {
        return;
    }

    queue->p_buffer = (uint8_t *)buffer;
    queue->item_count = item_count;
    queue->item_size = item_size;
    
    queue->in = 0;
    queue->out = 0;
}


/**
 * @brief  检查队列是否已满
 * @return 1:满, 0:未满
 */
uint8_t queue_is_full(my_queue_t *queue)
{
    if (queue == NULL) return 1;

    // 计算 in 的下一位索引,环形
    uint16_t next_in = queue->in + 1;
    if (next_in >= queue->item_count) {
        next_in = 0;
    }

    // 如果 in 的下一位是 out，则认为满 (保留一个空位)
    return (next_in == queue->out);
}

/**
 * @brief  非覆盖模式入队，队列满时入队失败
 * @param  p_src: 源数据指针
 * @return 1:成功, 0:失败(满)
 */
uint8_t queue_push(my_queue_t *queue, const void *p_src)
{
    if (queue_is_full(queue) || (p_src == NULL)) {
        return 0;
    }

    //计算写指针位置
    uint8_t * write_ptr = queue_get_write_ptr(queue);
    
    // 拷贝数据到缓冲区
    memcpy(write_ptr, p_src, queue->item_size);

    // 更新索引
    uint16_t next_in = queue->in + 1;
    if (next_in >= queue->item_count) {
        next_in = 0;
    }

    // 确保数据写入完成后再更新 in 
    queue->in = next_in;
    
    return 1;
}

/**
 * @brief  以覆盖模式入队
 *         如果队列不满，正常写入；
 *         如果队列已满，丢弃最旧的数据（头部覆盖尾部），写入新数据。
 * @param  p_src: 源数据指针
 * @return 1:总是成功
 */
uint8_t queue_push_overwrite(my_queue_t *queue, const void *p_src)
{
    if ((queue == NULL) || (p_src == NULL)) {
        return 0;
    }

    /* 检查队列是否满，如果满了，强行移动 out 指针，丢弃旧数据 */
    if (queue_is_full(queue)) {
        uint16_t next_out = queue->out + 1;
        if (next_out >= queue->item_count) {
            next_out = 0;
        }
        queue->out = next_out; //丢弃最老的一个数据
    }

   //计算写指针位置
    uint8_t* write_ptr = queue_get_write_ptr(queue);
    
    // 拷贝数据到缓冲区
    memcpy(write_ptr, p_src, queue->item_size);

    // 更新索引
    uint16_t next_in = queue->in + 1;
    if (next_in >= queue->item_count) {
        next_in = 0;
    }
    queue->in = next_in;

    return 1;
}


/**
 * @brief  出队 (消费者)
 * @param  p_dest: 目标数据缓冲区指针，如果为NULL则丢弃数据
 * @return 1:成功, 0:失败(空)
 */
uint8_t queue_pop(my_queue_t *queue, void *p_dest)
{
    if (queue_is_empty(queue)) {
        //art_printf("queue_pop fail: empty\r\n");
        return 0;
    }

    // 获取读指针位置
    uint8_t * read_ptr = queue_get_read_ptr(queue);

    // 如果提供了目标地址，则拷贝数据
    if (p_dest != NULL) {
        memcpy(p_dest, read_ptr, queue->item_size);
    }

    // 更新索引
    uint16_t next_out = queue->out + 1;
    if (next_out >= queue->item_count) {
        next_out = 0;
    }

    // 更新 out
    queue->out = next_out;
    return 1;
}

/**
 * @brief  移除指定数量的数据项 (丢弃)
 * @param  len: 要移除的数据项数量
 * @return 实际移除的数据项数量
 */
uint8_t queue_remove(my_queue_t *queue, uint16_t len)
{
    uint16_t avail = queue_get_counts(queue);
    len = MIN(len, avail);
    queue->out = (queue->out + len) % queue->item_count;
    return len;
}



/**
 * @brief  查看队头数据但不取出 (Peek)
 * @param  p_dest: 目标数据缓冲区指针
 * @return 1:成功, 0:失败(空)
 */
uint8_t queue_peek(my_queue_t *queue, void *p_dest)
{
    if (queue_is_empty(queue) || (p_dest == NULL)) {
        return 0;
    }

    uint8_t* read_ptr = queue_get_read_ptr(queue);
    
    // 仅拷贝数据，不移动 out 指针
    memcpy(p_dest, read_ptr, queue->item_size);

    return 1;
}

/**
 * @brief  查看队列中相对于读指针偏移位置的数据但不取出
 * @param  queue: 队列指针
 * @param  offset: 距离读指针的偏移量 (0表示队头第一个，1表示第二个...)
 * @param  p_dest: 数据拷贝目标缓冲区
 * @return 0:失败(队列空) n:实际查看的偏移量
 */
uint8_t queue_peek_at(my_queue_t *queue, uint16_t offset, void *p_dest)
{
    if (queue_is_empty(queue) || (p_dest == NULL)) {
        return 0;
    }

    uint16_t count = queue_get_counts(queue);
    
    // 检查偏移量是否超出当前有效数据范围
    uint16_t n = MIN(offset, count - 1);
    
    // 计算物理地址
    uint8_t* target_ptr = queue_get_at(queue, n);
    
    // 拷贝数据
    memcpy(p_dest, target_ptr, queue->item_size);

    return n;
}