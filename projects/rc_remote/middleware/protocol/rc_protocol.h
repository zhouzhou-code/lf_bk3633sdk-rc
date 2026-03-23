/**
 ****************************************************************************************
 * @file rc_protocol.h
 * @brief 遥控器通信协议 v2
 *
 * 帧格式 (7B overhead):
 *   [HEAD: CA BD] [INFO: 1B] [CMD: 1B] [SEQ: 1B] [PAYLOAD: nB] [CRC16: 2B]
 *
 *   HEAD     - 固定帧头 0xCA 0xBD
 *   INFO     - bit[7:3]=LEN(5bit, 0-31) | bit[2:0]=保留
 *   CMD      - 命令码 (隐含方向和设备类型)
 *   SEQ      - 序列号 (下行=seq, 上行=ack_seq)
 *   PAYLOAD  - 应用层数据 (最大25B)
 *   CRC16    - 校验范围: INFO + CMD + SEQ + PAYLOAD，小端存储
 ****************************************************************************************
 */
#ifndef _RC_PROTOCOL_H_
#define _RC_PROTOCOL_H_

#include <stdint.h>

/* ======================== 帧常量 ======================== */

#define PROTO_HEAD_0        0xCA
#define PROTO_HEAD_1        0xBD
#define PROTO_OVERHEAD      7       /* HEAD(2) + INFO(1) + CMD(1) + SEQ(1) + CRC(2) */
#define PROTO_MAX_PAYLOAD   25      /* 32 - 7 */

/* ======================== INFO 位域操作 ======================== */

#define PROTO_MAKE_INFO(len)    ((uint8_t)((len) << 3))
#define PROTO_INFO_LEN(info)    (((info) >> 3) & 0x1F)

/* ======================== 命令码 ======================== */

#define CMD_ESC_CTRL        0x31    /* REMOTE → ESC: 控制数据 */
#define CMD_BAT_QUERY       0x32    /* REMOTE → BATTERY: 电池查询 */
#define CMD_ESC_STATUS      0x61    /* ESC → REMOTE: 电控状态回传 */
#define CMD_BAT_STATUS      0x62    /* BATTERY → REMOTE: 电池状态回传 */

/* ======================== 模式 ======================== */

#define MODE_ASSIST         0x00    /* 助力模式 */
#define MODE_TRAINING       0x01    /* 训练模式(阻力) */



/* ======================== 应用层载荷定义 ======================== */

/* REMOTE → ESC 控制数据 (CMD=0x31, 3B) */
typedef struct {
    uint8_t  gear;          /* 输出功率档位 1~4 */
    uint8_t  mode;          /* MODE_ASSIST / MODE_TRAINING */
    uint8_t  throttle;      /* 油门值百分比 0~100 */
} __attribute__((packed)) esc_ctrl_data_t;

/* ESC → REMOTE 状态数据 (CMD=0x61, 6B) */
typedef struct {
    uint16_t speed;         /* 速度 (小端) */
    uint32_t mileage;       /* 里程 (小端) */
} __attribute__((packed)) esc_status_data_t;

/* BATTERY → REMOTE 状态数据 (CMD=0x62, 3B) */
typedef struct {
    uint8_t  temperature;   /* 温度 (偏移+40°C, 即0=−40°C) */
    uint8_t  soc;           /* 电量百分比 0~100 */
    uint8_t  status;        /* 状态标志位 */
} __attribute__((packed)) bat_status_data_t;

/* ======================== 传输层: 通用打包/解析 ======================== */

/**
 * @brief 通用帧打包
 * @param buf     输出缓冲区 (至少 7+plen 字节)
 * @param cmd     命令码
 * @param seq     序列号
 * @param payload 载荷数据 (可为NULL当plen=0)
 * @param plen    载荷长度
 * @return        帧总字节数 (7+plen)
 */
uint8_t proto_pack(uint8_t *buf, uint8_t cmd, uint8_t seq,
                   const void *payload, uint8_t plen);

/**
 * @brief 通用帧解析
 * @param buf     接收帧数据
 * @param len     帧长度
 * @param cmd     [out] 命令码
 * @param seq     [out] 序列号
 * @param payload [out] 载荷起始指针
 * @param plen    [out] 载荷长度
 * @return        0=成功, -1=校验失败
 */
int8_t proto_parse(const uint8_t *buf, uint8_t len,
                   uint8_t *cmd, uint8_t *seq,
                   const uint8_t **payload, uint8_t *plen);

/* CRC16-CCITT (poly=0x1021, init=0xFFFF) */
uint16_t proto_crc16(const uint8_t *data, uint8_t len);

/* ======================== 发送确认跟踪 ======================== */

typedef struct {
    uint8_t seq;            /* 当前发送seq (每帧自增, 自然回绕0-255) */
    uint8_t acked_seq;      /* 最后收到的ack_seq */
    uint8_t change_seq;     /* 数据变化时记录的seq */
    uint8_t pending;        /* 1=待确认, 0=已确认 */
} proto_tracker_t;

void    tracker_init(proto_tracker_t *t);
uint8_t tracker_next_seq(proto_tracker_t *t);
void    tracker_mark_pending(proto_tracker_t *t);
void    tracker_on_ack(proto_tracker_t *t, uint8_t ack_seq);
uint8_t tracker_is_pending(const proto_tracker_t *t);

#endif /* _RC_PROTOCOL_H_ */
