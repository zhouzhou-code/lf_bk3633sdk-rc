/**
 ****************************************************************************************
 * @file rc_protocol.h
 * @brief 遥控器通信协议
 *
 * 帧格式:
 *   [HEAD: A5 5A] [LEN: 1B] [CMD: 1B] [SEQ: 1B] [PAYLOAD: nB] [CRC16: 2B] [TAIL: BB]
 *
 *   HEAD     - 固定帧头 0xA5 0x5A
 *   LEN      - payload字节数（不含HEAD/CMD/SEQ/CRC/TAIL）
 *   CMD      - 命令码，遥控下行0x3x，电机上行0x6x
 *   SEQ      - 序列号，下行为发送seq，上行为确认ack_seq
 *   PAYLOAD  - 数据域
 *   CRC16    - 校验范围: LEN + CMD + SEQ + PAYLOAD，小端存储
 *   TAIL     - 固定帧尾 0xBB
 *
 * 确认机制（ACK payload异步回传）:
 *   遥控主动发送，主控通过RF的ACK payload回传数据。
 *   由于ACK payload至少延迟一帧，遥控通过seq/ack_seq匹配确认:
 *
 *   1. 遥控每帧携带递增的seq
 *   2. 主控收到后将seq存入ack_seq，随下一次ACK回传
 *   3. 遥控收到ack_seq后与记录的change_seq比较:
 *      - ack_seq >= change_seq → 数据已确认，停止重发
 *      - ack_seq <  change_seq → 数据未确认，继续发送
 *   4. seq为uint8_t自然回绕(0-255)，用(int8_t)差值比较处理溢出
 *
 *   示例（油门从0变到500）:
 *     TX seq=10 throttle=500  →  主控收到, 存ack_seq=10
 *     TX seq=11 throttle=500  ←  ACK ack_seq=10, 确认! 停止重发
 *
 *   示例（中间丢包）:
 *     TX seq=10 throttle=500  →  丢包
 *     TX seq=11 throttle=500  →  主控收到, 存ack_seq=11
 *     TX seq=12 throttle=500  ←  ACK ack_seq=11, 确认! 停止重发
 *
 ****************************************************************************************
 */
#ifndef _RC_PROTOCOL_H_
#define _RC_PROTOCOL_H_

#include <stdint.h>

/* ======================== 帧常量 ======================== */

#define PROTO_HEAD_0        0xA5
#define PROTO_HEAD_1        0x5A
#define PROTO_TAIL          0xBB
#define PROTO_MAX_PAYLOAD   16

/* ======================== 命令码 ======================== */

#define CMD_RC_CTRL         0x31    /* 遥控 → 主控: 控制数据 */
#define CMD_MC_STATUS       0x61    /* 主控 → 遥控: 状态回传 */

/* ======================== 模式 ======================== */

#define MODE_ASSIST         0x00    /* 助力模式 */
#define MODE_TRAINING       0x01    /* 训练模式(阻力) */

/* ======================== 载荷定义 ======================== */

/* 遥控 → 主控: 档位gear决定最大输出功率 1=25%, 2=50%, 3=75%, 4=100% */
typedef struct {
    uint8_t  gear;          /* 档位 1-4 */
    uint8_t  mode;          /* MODE_ASSIST(助力模式) / MODE_TRAINING(训练模式) */
    uint8_t  throttle;      /* 油门值 */
} __attribute__((packed)) rc_ctrl_t;

/* 主控 → 遥控: 通过ACK payload回传 */
typedef struct {
    uint16_t speed;         /* 速度 */
    uint32_t mileage;       /* 里程 */
} __attribute__((packed)) mc_status_t;

/* ======================== 帧打包/解析 ======================== */

/*
 * 打包下行控制帧
 * @param buf  输出缓冲区
 * @param seq  当前发送序列号
 * @param ctrl 控制数据
 * @return     帧总字节数
 */
uint8_t proto_pack_ctrl(uint8_t *buf, uint8_t seq, const rc_ctrl_t *ctrl);

/*
 * 解析上行状态帧 (从ACK payload中提取)
 * @param buf     接收帧数据
 * @param len     帧长度
 * @param ack_seq [out] 主控确认的seq
 * @param status  [out] 状态数据
 * @return        0=成功, -1=校验失败
 */
int8_t proto_parse_status(const uint8_t *buf, uint8_t len,
                          uint8_t *ack_seq, mc_status_t *status);

/* CRC16-CCITT (poly=0x1021, init=0xFFFF) */
uint16_t proto_crc16(const uint8_t *data, uint8_t len);

/* ======================== 发送确认跟踪 ======================== */

/*
 * 跟踪器: 管理seq自增 + 油门确认状态
 *
 * 使用流程:
 *   1. 油门变化时调用 tracker_mark_pending()  → 记录change_seq, 标记待确认
 *   2. 发送时调用    tracker_next_seq()       → 获取递增seq
 *   3. 收到ACK调用   tracker_on_ack(ack_seq)  → 比对, 自动清除pending
 *   4. 查询          tracker_is_pending()     → 非0=需要继续发送
 */
typedef struct {
    uint8_t seq;            /* 当前发送seq (每帧自增, 自然回绕0-255) */
    uint8_t acked_seq;      /* 最后收到的ack_seq */
    uint8_t change_seq;     /* 油门变化时记录的seq */
    uint8_t pending;        /* 1=油门待确认, 0=已确认 */
} proto_tracker_t;

void    tracker_init(proto_tracker_t *t);
uint8_t tracker_next_seq(proto_tracker_t *t);       /* 返回新seq */
void    tracker_mark_pending(proto_tracker_t *t);    /* 油门变化时调用 */
void    tracker_on_ack(proto_tracker_t *t, uint8_t ack_seq);
uint8_t tracker_is_pending(const proto_tracker_t *t); /* 非0=待确认 */

#endif /* _RC_PROTOCOL_H_ */
