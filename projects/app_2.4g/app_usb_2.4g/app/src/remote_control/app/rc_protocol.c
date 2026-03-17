/**
 ****************************************************************************************
 * @file rc_protocol.c
 * @brief 遥控器通信协议 - 打包/解析/确认跟踪
 ****************************************************************************************
 */

#include "rc_protocol.h"
#include <string.h>

/* ======================== CRC16 ======================== */

/* CRC16-CCITT: poly=0x1021, init=0xFFFF, 逐位计算 */
uint16_t proto_crc16(const uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return crc;
}

/* ======================== 打包 ======================== */

/*
 * 下行控制帧:
 * [A5 5A] [LEN] [CMD_RC_CTRL] [SEQ] [rc_ctrl_t] [CRC16_L CRC16_H] [BB]
 *  idx: 0  1     2             3      4           4+plen   +1        +2
 *
 * CRC计算范围: buf[2] ~ buf[4+plen-1], 即 LEN+CMD+SEQ+PAYLOAD
 */
uint8_t proto_pack_ctrl(uint8_t *buf, uint8_t seq, const rc_ctrl_t *ctrl)
{
    uint8_t idx = 0;
    uint8_t plen = sizeof(rc_ctrl_t);

    /* 帧头 */
    buf[idx++] = PROTO_HEAD_0;      /* [0] */
    buf[idx++] = PROTO_HEAD_1;      /* [1] */

    /* LEN + CMD + SEQ */
    buf[idx++] = plen;              /* [2] payload长度 */
    buf[idx++] = CMD_RC_CTRL;       /* [3] 命令码 */
    buf[idx++] = seq;               /* [4] 序列号 */

    /* PAYLOAD */
    memcpy(&buf[idx], ctrl, plen);  /* [5 ~ 5+plen-1] */
    idx += plen;

    /* CRC16 (小端: 低字节在前) */
    uint16_t crc = proto_crc16(&buf[2], 1 + 1 + 1 + plen); /* LEN+CMD+SEQ+PAYLOAD */
    buf[idx++] = (uint8_t)(crc & 0xFF);
    buf[idx++] = (uint8_t)(crc >> 8);

    /* 帧尾 */
    buf[idx++] = PROTO_TAIL;

    return idx; /* 总帧长 = 2+1+1+1+plen+2+1 */
}

/* ======================== 解析 ======================== */

/*
 * 上行状态帧 (ACK payload):
 * [A5 5A] [LEN] [CMD_MC_STATUS] [ACK_SEQ] [mc_status_t] [CRC16_L CRC16_H] [BB]
 *
 * 校验顺序: 长度 → 帧头 → 命令码 → payload长度 → 帧尾 → CRC → 提取数据
 */
int8_t proto_parse_status(const uint8_t *buf, uint8_t len,
                          uint8_t *ack_seq, mc_status_t *status)
{
    uint8_t plen = sizeof(mc_status_t);
    /* 期望帧长: head(2)+len(1)+cmd(1)+seq(1)+payload+crc(2)+tail(1) */
    uint8_t expect_len = 2 + 1 + 1 + 1 + plen + 2 + 1;

    if (len < expect_len)                                   return -1;
    if (buf[0] != PROTO_HEAD_0 || buf[1] != PROTO_HEAD_1)  return -1;
    if (buf[3] != CMD_MC_STATUS)                            return -1;
    if (buf[2] != plen)                                     return -1;
    if (buf[expect_len - 1] != PROTO_TAIL)                  return -1;

    /* CRC校验 */
    uint16_t crc_calc = proto_crc16(&buf[2], 1 + 1 + 1 + plen);
    uint16_t crc_recv = buf[5 + plen] | ((uint16_t)buf[5 + plen + 1] << 8);
    if (crc_calc != crc_recv)                               return -1;

    /* 提取数据 */
    *ack_seq = buf[4];
    memcpy(status, &buf[5], plen);
    return 0;
}

/* ======================== 从机侧接口 ======================== */

/* 解析遥控下行控制帧 (从机使用) */
int8_t proto_parse_ctrl(const uint8_t *buf, uint8_t len,
                        uint8_t *seq, rc_ctrl_t *ctrl)
{
    uint8_t plen = sizeof(rc_ctrl_t);
    uint8_t expect = 2 + 1 + 1 + 1 + plen + 2 + 1;

    if (len < expect)                                       return -1;
    if (buf[0] != PROTO_HEAD_0 || buf[1] != PROTO_HEAD_1)  return -1;
    if (buf[3] != CMD_RC_CTRL)                              return -1;
    if (buf[2] != plen)                                     return -1;
    if (buf[expect - 1] != PROTO_TAIL)                      return -1;

    uint16_t crc_calc = proto_crc16(&buf[2], 1 + 1 + 1 + plen);
    uint16_t crc_recv = buf[5 + plen] | ((uint16_t)buf[5 + plen + 1] << 8);
    if (crc_calc != crc_recv)                               return -1;

    *seq = buf[4];
    memcpy(ctrl, &buf[5], plen);
    return 0;
}

/* 打包上行状态帧 (从机装入ACK payload) */
uint8_t proto_pack_status(uint8_t *buf, uint8_t ack_seq, const mc_status_t *status)
{
    uint8_t idx = 0;
    uint8_t plen = sizeof(mc_status_t);

    buf[idx++] = PROTO_HEAD_0;
    buf[idx++] = PROTO_HEAD_1;
    buf[idx++] = plen;
    buf[idx++] = CMD_MC_STATUS;
    buf[idx++] = ack_seq;

    memcpy(&buf[idx], status, plen);
    idx += plen;

    uint16_t crc = proto_crc16(&buf[2], 1 + 1 + 1 + plen);
    buf[idx++] = (uint8_t)(crc & 0xFF);
    buf[idx++] = (uint8_t)(crc >> 8);

    buf[idx++] = PROTO_TAIL;
    return idx;
}

void tracker_init(proto_tracker_t *t)
{
    t->seq        = 0;
    t->acked_seq  = 0;
    t->change_seq = 0;
    t->pending    = 0;
}

/* 获取下一个seq (先自增再返回) */
uint8_t tracker_next_seq(proto_tracker_t *t)
{
    return ++t->seq;
}

/* 油门值变化时调用: 记录当前seq为change_seq, 标记待确认 */
void tracker_mark_pending(proto_tracker_t *t)
{
    t->change_seq = t->seq;
    t->pending    = 1;
}

/*
 * 收到ACK时调用
 * 用(int8_t)差值处理uint8_t回绕:
 *   (int8_t)(ack_seq - change_seq) >= 0 等价于 ack_seq >= change_seq (模256)
 */
void tracker_on_ack(proto_tracker_t *t, uint8_t ack_seq)
{
    t->acked_seq = ack_seq;

    if (t->pending && (int8_t)(ack_seq - t->change_seq) >= 0) {
        t->pending = 0;
    }
}

/* 返回非0表示油门待确认, 需继续发送 */
uint8_t tracker_is_pending(const proto_tracker_t *t)
{
    return t->pending;
}
