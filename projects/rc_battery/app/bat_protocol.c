#include "bat_protocol.h"
#include "my_drv_uart.h"
#include "user_config.h"
#include <string.h>

static Bat_Status_t g_last_status_pkt;
static bool g_status_pending = false;

static uint8_t g_pair_cmd_addr = 0;
static bool g_pair_cmd_pending = false;

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

static void bat_protocol_push_status(const BatteryDynamicInfo_t *p_dynamic_info)
{
    Bat_Status_t bat_status = {
        .header = 0xAA,
        .cmd = UPCMD_DYNAMIC,
        .length = 0x03,
        .temperature = p_dynamic_info->temp_cell,
        .soc = p_dynamic_info->soc,
        .status = (p_dynamic_info->status != 0) ? 0x01 : 0x00,
    };

    bat_status.crc16 = crc16_modbus((const uint8_t *)&bat_status, sizeof(Bat_Status_t) - 2);
    memcpy(&g_last_status_pkt, &bat_status, sizeof(Bat_Status_t));
    g_status_pending = true;
}

static void bat_protocol_push_pair_cmd(uint8_t addr)
{
    g_pair_cmd_addr = addr;
    g_pair_cmd_pending = true;
}

void Protocol_ParseByte(my_queue_t* uart_rx_queue)
{
    static uint8_t temp_parse_buf[sizeof(BatteryDynamicInfo_t)];
    my_queue_t *q = uart_rx_queue;

    while (1) {
        if (queue_get_counts(q) < 5) {
            break;
        }

        uint8_t h1, h2;
        queue_peek_at(q, 0, &h1);
        queue_peek_at(q, 1, &h2);
        if (h1 != FRAME_HEADER_1 || h2 != FRAME_HEADER_2) {
            queue_remove(q, 1);
            continue;
        }

        uint8_t cmd_byte;
        queue_peek_at(q, 2, &cmd_byte);
        if (cmd_byte != UPCMD_DYNAMIC && cmd_byte != PAIR_CMD) {
            queue_remove(q, 1);
            continue;
        }

        uint8_t addr_byte;
        uint8_t data_len;
        queue_peek_at(q, 3, &addr_byte);
        queue_peek_at(q, 4, &data_len);

        if (PROTOCOL_OVERHEAD + data_len > sizeof(temp_parse_buf)) {
            queue_remove(q, 1);
            continue;
        }

        if (queue_get_counts(q) < 6) {
            break;
        }

        uint8_t recv_chksum;
        queue_peek_at(q, 5, &recv_chksum);
        uint8_t calc_sum = h1 + h2 + cmd_byte + addr_byte + data_len;
        if (recv_chksum != calc_sum) {
            queue_remove(q, 1);
            continue;
        }

        uint16_t total_frame_len = PROTOCOL_OVERHEAD + data_len;
        if (queue_get_counts(q) < total_frame_len) {
            break;
        }

        for (uint16_t i = 0; i < total_frame_len; i++) {
            queue_pop(q, &temp_parse_buf[i]);
        }

        uint16_t recv_crc =
            (uint16_t)(temp_parse_buf[total_frame_len - 1] << 8) | (temp_parse_buf[total_frame_len - 2] & 0xFF);
        uint16_t calc_crc = crc16_modbus(temp_parse_buf, total_frame_len - 2);
        if (recv_crc != calc_crc) {
            continue;
        }

        if (cmd_byte == UPCMD_DYNAMIC) {
            if (total_frame_len == sizeof(BatteryDynamicInfo_t)) {
                const BatteryDynamicInfo_t *p_dynamic_info = (const BatteryDynamicInfo_t *)&temp_parse_buf[0];
                bat_protocol_push_status(p_dynamic_info);
            }
        } else if (cmd_byte == PAIR_CMD) {
            // 支持两种格式：
            // 1. data_len=0: AA 55 AA 00 00 [校验和] [CRC16]
            // 2. data_len=1: AA 55 AA 00 01 [校验和] 00 [CRC16]
            if (data_len == 0) {
                bat_protocol_push_pair_cmd(addr_byte);
            } else if (data_len == 1 && temp_parse_buf[6] == 0x00) {
                bat_protocol_push_pair_cmd(addr_byte);
            }
        }
    }
}

bool bat_protocol_take_status(Bat_Status_t *out_status)
{
    if (!out_status || !g_status_pending) {
        return false;
    }

    memcpy(out_status, &g_last_status_pkt, sizeof(Bat_Status_t));
    g_status_pending = false;
    return true;
}

bool bat_protocol_take_pair_cmd(uint8_t *out_addr)
{
    if (!g_pair_cmd_pending) {
        return false;
    }

    if (out_addr) {
        *out_addr = g_pair_cmd_addr;
    }
    g_pair_cmd_pending = false;
    return true;
}

void bat_protocol_send_pair_resp(uint8_t addr, uint8_t result_code)
{
    uint8_t frame[9];
    uint16_t crc;

    frame[0] = FRAME_HEADER_1;
    frame[1] = FRAME_HEADER_2;
    frame[2] = PAIR_RESP;
    frame[3] = addr;
    frame[4] = 1;
    frame[5] = (uint8_t)(frame[0] + frame[1] + frame[2] + frame[3] + frame[4]);
    frame[6] = result_code;

    crc = crc16_modbus(frame, sizeof(frame) - 2);
    frame[7] = (uint8_t)(crc & 0xFF);
    frame[8] = (uint8_t)(crc >> 8);

    uart_send(frame, sizeof(frame));
}
