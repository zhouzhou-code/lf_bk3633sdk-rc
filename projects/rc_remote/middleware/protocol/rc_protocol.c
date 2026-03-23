/**
 ****************************************************************************************
 * @file rc_protocol.c
 * @brief 遥控器通信协议 v2 - 通用打包/解析 + 确认跟踪
 *
 * 帧格式 (7B overhead):
 *   [CA BD] [INFO] [CMD] [SEQ] [PAYLOAD] [CRC16]
 *   INFO = bit[7:3]=LEN(5bit) | bit[2:0]=保留
 *   CRC范围: INFO + CMD + SEQ + PAYLOAD
 ****************************************************************************************
 */

#include "rc_protocol.h"
#include <string.h>

/* ======================== CRC16 ======================== */

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

/* ======================== 通用打包 ======================== */

/*
 * [CA BD] [INFO] [CMD] [SEQ] [PAYLOAD...] [CRC_L] [CRC_H]
 *  0   1    2      3     4     5..5+plen-1  5+plen  6+plen
 *
 * CRC范围: buf[2] ~ buf[4+plen], 即 INFO+CMD+SEQ+PAYLOAD
 * 总帧长: 7 + plen
 */
uint8_t proto_pack(uint8_t *buf, uint8_t cmd, uint8_t seq,
                   const void *payload, uint8_t plen)
{
    buf[0] = PROTO_HEAD_0;
    buf[1] = PROTO_HEAD_1;
    buf[2] = PROTO_MAKE_INFO(plen);
    buf[3] = cmd;
    buf[4] = seq;

    if (plen > 0 && payload != NULL) {
        memcpy(&buf[5], payload, plen);
    }

    uint16_t crc = proto_crc16(&buf[2], 1 + 1 + 1 + plen);  /* INFO+CMD+SEQ+PAYLOAD */
    buf[5 + plen]     = (uint8_t)(crc & 0xFF);
    buf[5 + plen + 1] = (uint8_t)(crc >> 8);

    return PROTO_OVERHEAD + plen;
}

/* ======================== 通用解析 ======================== */

int8_t proto_parse(const uint8_t *buf, uint8_t len,
                   uint8_t *cmd, uint8_t *seq,
                   const uint8_t **payload, uint8_t *plen)
{
    /* 最小帧长 = 7 (无payload) */
    if (len < PROTO_OVERHEAD)                               return -1;
    if (buf[0] != PROTO_HEAD_0 || buf[1] != PROTO_HEAD_1)  return -1;

    uint8_t pl = PROTO_INFO_LEN(buf[2]);

    /* 校验帧长 */
    if (len < (uint8_t)(PROTO_OVERHEAD + pl))               return -1;

    /* CRC校验 */
    uint16_t crc_calc = proto_crc16(&buf[2], 1 + 1 + 1 + pl);
    uint16_t crc_recv = buf[5 + pl] | ((uint16_t)buf[5 + pl + 1] << 8);
    if (crc_calc != crc_recv)                               return -1;

    /* 提取字段 */
    *cmd     = buf[3];
    *seq     = buf[4];
    *payload = (pl > 0) ? &buf[5] : NULL;
    *plen    = pl;
    return 0;
}

/* ======================== 确认跟踪 ======================== */

void tracker_init(proto_tracker_t *t)
{
    t->seq        = 0;
    t->acked_seq  = 0;
    t->change_seq = 0;
    t->pending    = 0;
}

uint8_t tracker_next_seq(proto_tracker_t *t)
{
    return ++t->seq;
}

void tracker_mark_pending(proto_tracker_t *t)
{
    t->change_seq = t->seq;
    t->pending    = 1;
}

void tracker_on_ack(proto_tracker_t *t, uint8_t ack_seq)
{
    t->acked_seq = ack_seq;
    if (t->pending && (int8_t)(ack_seq - t->change_seq) >= 0) {
        t->pending = 0;
    }
}

uint8_t tracker_is_pending(const proto_tracker_t *t)
{
    return t->pending;
}
