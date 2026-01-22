#ifndef __BAT_PROTOCOL_H
#define __BAT_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <kfifo.h>


// 协议参数
#define FRAME_HEADER_1   0xAA        // 帧头字节1
#define FRAME_HEADER_2   0x55        // 帧头字节2
#define PAIR_CMD         0xAA        // 配对命令码
#define PAIR_RESP        0xBB        // 配对响应码

//遥控下行命令码
#define DOWNCMD_GET_STATIC   0x01        // 获取静态信息
#define DOWNCMD_GET_DYNAMIC  0x02        // 获取动态信息
#define DOWNCMD_GET_CELL_V   0x03        // 获取电芯电压
#define DOWNCMD_OPERATION    0x04        // 操作指令
#define DOWNCMD_CHARGE_INFO  0x40        // 充电信息

//电池上行命令码
#define UPCMD_STATIC      0x81        // 静态信息响应
#define UPCMD_DYNAMIC     0x82        // 动态信息响应
#define UPCMD_CELL_V      0x83        // 电芯电压响应
#define UPCMD_OPERATION   0x84        // 操作响应
#define UPCMD_CHARGE      0xC0        // 充电信息响应


// 缓冲区大小
#define UART_RX_BUF_SIZE 512         // 串口接收缓冲区
#define UART_TX_BUF_SIZE 512         // 串口发送缓冲区
#define WIRELESS_BUF_SIZE 128        // 无线缓冲区

//协议固定开销  帧头(2) + 命令码(1) + 地址(1) + 长度(1) + 帧头校验和(1) + CRC16(2)
#define PROTOCOL_OVERHEAD 8  


//帧结构体
#pragma pack(1)
typedef struct {
    uint8_t header[2];     // 帧头 0xAA 0x55
    uint8_t cmd;           // 命令码
    uint8_t addr;          // 设备地址
    uint8_t length;        // 数据长度
    uint8_t chksum;        // 帧头校验和
    uint8_t *data;         // 数据段指针
    uint16_t crc16;        // CRC16校验值
} Frame_t;
#pragma pack()


// 电池静态信息结构体 30字节
#pragma pack(1)
typedef struct {
    uint8_t sn[14];        // SN编码 ASCII
    uint8_t ver_main;      // 主版本号
    uint8_t ver_minor;     // 从版本号
    uint16_t ver_build;    // 编译版本号
    uint16_t ver_hardware; // 硬件版本号
    uint16_t nominal_cap;  // 额定容量 (10mAh/bit)
} BatteryStaticInfo_t;
#pragma pack()

// 电池动态信息结构体 包含所有帧 则54字节 //暂时只需要soc转发
#pragma pack(1)
typedef struct {
    uint8_t header[2];     // 帧头 0xAA 0x55
    uint8_t cmd;           // 命令码
    uint8_t addr;          // 设备地址
    uint8_t length;        // 数据长度
    uint8_t chksum;        // 帧头校验和

    uint32_t volt_bat;     // 电池电压(mV)
    int32_t curr_bat;      // 电池电流(mA)
    uint8_t temp_cell;     // 电芯温度(偏移40℃)
    uint8_t temp_bms;      // 电池板温
    uint16_t remain_cap;   // 剩余容量(10mAh/bit)
    uint8_t soc;           // SOC(%)
    uint8_t soh;           // SOH(%)
    uint32_t sop;          // SOP值(W)
    uint16_t cycle;        // 循环数
    uint32_t volt_chg;     // 充电请求电压(mV)
    int32_t curr_chg;      // 充电请求电流(mA)
    uint32_t alarm;        // 告警标志
    uint32_t protect;      // 保护标志
    uint32_t status;       // 状态标志
    uint32_t volt_pack;    // Pack端电压(mV)
    uint16_t soc_q10;      // 高精度SOC(0.1%/bit)

    uint16_t crc16;        // CRC16校验值

} BatteryDynamicInfo_t;
#pragma pack()


// 操作指令枚举
typedef enum {
    OP_DISCHARGE_ON  = 0x01,    // 开主放电管
    OP_DISCHARGE_OFF = 0x02,    // 关主放电管
    OP_UNLOCK        = 0x06,    // 电调解锁
    OP_LOCK          = 0x07     // 电调加锁
} OperationCmd_t;

// 协议解析器状态
typedef enum {
    PARSE_IDLE = 0,        // 空闲状态
    PARSE_HEADER1,         // 接收帧头1
    PARSE_HEADER2,         // 接收帧头2
    PARSE_CMD,             // 接收命令码
    PARSE_ADDR,            // 接收地址
    PARSE_LENGTH,          // 接收数据长度
    PARSE_CHKSUM,          // 接收帧头校验和
    PARSE_DATA,            // 接收数据段
    PARSE_CRC1,            // 接收CRC低字节
    PARSE_CRC2             // 接收CRC高字节
} ParseState_t;

// 协议解析器结构体
typedef struct {
    ParseState_t state;
    uint8_t data_index;
    uint8_t expected_length;
    uint8_t buffer[256];
    uint32_t last_rx_time;
    Frame_t current_frame;
} ProtocolParser_t;


extern uint8_t temp_Parsebuf[sizeof(BatteryDynamicInfo_t)];

void Protocol_ParseByte(my_queue_t* uart_rx_queue);

#endif /* __BAT_PROTOCOL_H */
