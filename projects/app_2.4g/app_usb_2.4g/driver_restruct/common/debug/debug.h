/**
 ****************************************************************************************
 * @file debug.h
 * @brief Debug utilities for register dump, hex print, and VOFA+ JustFloat protocol
 ****************************************************************************************
 */
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdint.h>

void debug_print_rf_registers(void);
void debug_print_txrx_addr(void);
void debug_print_hex(void *msg, void *dat, int len);

// VOFA+ JustFloat协议
// ch_count: 通道数量，data: float数组指针
void vofa_senddata(float *data, uint8_t ch_count);

#endif // _DEBUG_H_
