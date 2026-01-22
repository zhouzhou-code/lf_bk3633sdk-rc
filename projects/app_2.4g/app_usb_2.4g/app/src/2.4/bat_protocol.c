
#include "bat_protocol.h"


uint8_t checksum8(const uint8_t *data, uint8_t length)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum & 0xFF;
}
uint16_t crc16_modbus(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}


uint8_t temp_Parsebuf[sizeof(BatteryDynamicInfo_t)];  // 临时线性缓冲区，用于存放解出来的一整包
void Protocol_ParseByte(my_queue_t* uart_rx_queue)
{
    my_queue_t *q = uart_rx_queue; // 你的队列实例
    while (1)
    {
        // 队列里的数据够不够协议头的最小长度 (至少要能看到 length 字节)
        // Header(2) + Cmd(1) + Addr(1) + Len(1) = 5 字节
        if (queue_get_counts(q) < 5) {
            break; // 数据不够，直接退出，等下次
        }

        // 检查帧头 (AA 55)
        uint8_t h1,h2;
        queue_peek_at(q, 0, &h1);
        queue_peek_at(q, 1, &h2);

        if (h1 != FRAME_HEADER_1 || h2 != FRAME_HEADER_2) {
            queue_remove(q, 1);//头不对，移除1个字节，继续下次循环找头
            continue; 
        }
        // 3. 头匹配了，查看命令码，这个解包只处理动态信息
        uint8_t cmd_byte;
        queue_peek_at(q, 2, &cmd_byte); // 读取命令码
        if(cmd_byte != UPCMD_DYNAMIC){
            //不是我们要的包，移除1个字节，继续下次循环找头
            queue_remove(q, 1);//不是我们要的包，移除1个字节，继续下次循环找头
            continue; 
        }
        uint8_t addr_byte;
        queue_peek_at(q, 3, &addr_byte);
        // 3. 命令码匹配了，查看长度字节 (Offset 4)
        uint8_t data_len;
        queue_peek_at(q, 4, &data_len);
        uint8_t recv_chksum;
        queue_peek_at(q, 5, &recv_chksum);

        uint8_t calc_sum = h1 + h2 + cmd_byte + addr_byte + data_len;
        if(recv_chksum != calc_sum){
            queue_remove(q, 1);//帧头校验和错误，移除1个字节，继续下次循环找头
            continue; 
        }

        // 4. 计算这一包完整数据应该有多长
        uint16_t total_frame_len = PROTOCOL_OVERHEAD + data_len;

        // 5. 检查队列里是否有足够的数据
        if (queue_get_counts(q) < total_frame_len-1)
        {
            // 这是一个断包（还没收完），保留数据不动，退出函数
            break; 
        }

        //数据完整,从队列里Pop出来到线性数组
        for (uint16_t i = 0; i < total_frame_len; i++){
            queue_pop(q, &temp_Parsebuf[i]);
        }

        // 校验CRC16
        //低字节在前，高字节在后
        uint16_t recv_crc = (uint16_t) ((uint16_t)temp_Parsebuf[total_frame_len - 1] << 8) | (temp_Parsebuf[total_frame_len - 2]&0xFF);
        uint16_t calc_crc = crc16_modbus(temp_Parsebuf, total_frame_len - 2);
        if(recv_crc != calc_crc){
            continue; // CRC校验失败，丢弃这包数据，继续下次循环找头
        }else{
            // 成功接收并校验一包完整数据，进行处理
        }
    }
}