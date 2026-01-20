/*************************************************************
 * @file        driver_bk24.h
 * @brief       BK2423/BK2401 无线收发器驱动头文件
 * @author      lvjinxia
 * @version     V1.0
 * @date        2018-09-06
 * @par
 * @attention   BK2423/BK2401是一款2.4GHz无线收发芯片，支持1Mbps、2Mbps、250kbps速率
 *              本驱动提供寄存器定义和基本操作接口
 */
#ifndef __DRIVER_BK24_H__
#define __DRIVER_BK24_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//#include "BK_System.h"
#include "Types.h"

/* BK2423寄存器映射基地址 */
#define APB_BK24_BASE            0x00805000

/* 寄存器访问宏：将APB_BK24_BASE定义为32位指针，通过数组索引访问寄存器 */
#define  XBYTE      ((volatile uint32 *)APB_BK24_BASE)

/* 地址定义（相对偏移量） */
#define  ADDR_BASE_TRX          0x00    /* 收发器寄存器基地址 */
#define  ADDR_BASE_XRAM         0x00    /* XRAM基地址（可能与收发器共享） */

/* ============== 配置寄存器组 ============== */
/* CONFIG - 配置寄存器 (0x00) */
#define  TRX_CONFIG             XBYTE[0x00]  /* 收发器主配置寄存器 */
/* 位定义: 
 * bit[6]: MASK_RX_DR - 屏蔽接收数据就绪中断(1=屏蔽，0=允许且低电平触发)
 * bit[5]: MASK_TX_DS - 屏蔽发送完成中断(1=屏蔽，0=允许且低电平触发)
 * bit[4]: MASK_MAX_RT - 屏蔽最大重传次数中断(1=屏蔽，0=允许且低电平触发)
 * bit[3]: EN_CRC - 启用CRC校验
 * bit[2]: CRCO - CRC编码方案 (0=1字节, 1=2字节)
 * bit[1]: PWR_UP - 电源控制 (0=掉电, 1=上电)
 * bit[0]: PRIM_RX - 收发模式 (0=PTX发送, 1=PRX接收)
 */
 /*默认0x0c= |0|1|1|0|0 */

#define  TRX_EN_AA              XBYTE[0x01]  /* 自动应答使能寄存器 */
/* 位定义:
 * bit[5:0]: ENAA_P5~ENAA_P0 - 分别使能6个数据管道的自动应答功能;默认配置值111111(二进制)=0x3f
 */

#define  TRX_EN_RXADDR          XBYTE[0x02]  /* 接收地址使能寄存器 */
/* 位定义:
 * bit[5:0]: ERX_P5~ERX_P0 - 分别使能6个数据管道的接收地址;默认配置值111111(二进制)=0x3f
 */

#define  TRX_SETUP_AW           XBYTE[0x03]  /* 地址宽度设置寄存器 */
/* 位定义:
 * bit[1:0]: AW - 地址宽度
 *           00: 非法
 *           01: 3字节
 *           10: 4字节
 *           11: 5字节
 */

#define  TRX_SETUP_RETR         XBYTE[0x04]  /* 自动重传设置寄存器 */
/* 位定义:
 * bit[7:4]: ARD - 自动重传延迟 (0000=250μs ~ 1111=4000μs)
 * bit[3:0]: ARC - 自动重传次数 (0000=禁用 ~ 1111=15次)
 */

#define  TRX_RF_CH              XBYTE[0x05]  /* RF频道设置寄存器 */
/* 位定义:
 * bit[7]:   自动频率选择 (0=XVR使用cur_cfg_chn, 1=使用RF_CH[6:0])
 * bit[6:0]: RF_CH - RF频率通道 (0-127)
 */

#define  TRX_RF_SETUP           XBYTE[0x06]  /* RF参数设置寄存器 */ 
/* 位定义:
 * bit[5]: En_250k_rate - 启用250kbps速率
 * bit[3]: RF_DR - 数据速率 (与bit[5]组合决定速率)
 * 组合: 00=1Mbps, 01=2Mbps, 10=250kbps, 11=保留
 */

/* ============== 地址寄存器组 ============== */
/* 接收地址寄存器 - 管道0 (5字节) */
#define  TRX_RX_ADDR_P0_0       XBYTE[0x07]  /* RX_ADDR_P0 字节0 (LSB) */
#define  TRX_RX_ADDR_P0_1       XBYTE[0x08]  /* RX_ADDR_P0 字节1 */
#define  TRX_RX_ADDR_P0_2       XBYTE[0x09]  /* RX_ADDR_P0 字节2 */
#define  TRX_RX_ADDR_P0_3       XBYTE[0x0a]  /* RX_ADDR_P0 字节3 */
#define  TRX_RX_ADDR_P0_4       XBYTE[0x0b]  /* RX_ADDR_P0 字节4 (MSB) */

/* 接收地址寄存器 - 管道1 (5字节) */
#define  TRX_RX_ADDR_P1_0       XBYTE[0x0c]  /* RX_ADDR_P1 字节0 (LSB) */
#define  TRX_RX_ADDR_P1_1       XBYTE[0x0d]  /* RX_ADDR_P1 字节1 */
#define  TRX_RX_ADDR_P1_2       XBYTE[0x0e]  /* RX_ADDR_P1 字节2 */
#define  TRX_RX_ADDR_P1_3       XBYTE[0x0f]  /* RX_ADDR_P1 字节3 */
#define  TRX_RX_ADDR_P1_4       XBYTE[0x10]  /* RX_ADDR_P1 字节4 (MSB) */

/* 接收地址寄存器 - 管道2-5 (仅一个LSB字节) */
#define  TRX_RX_ADDR_P2         XBYTE[0x11]  /* RX_ADDR_P2 (仅LSB, MSB部分与P1相同) */
#define  TRX_RX_ADDR_P3         XBYTE[0x12]  /* RX_ADDR_P3 (仅LSB, MSB部分与P1相同) */
#define  TRX_RX_ADDR_P4         XBYTE[0x13]  /* RX_ADDR_P4 (仅LSB, MSB部分与P1相同) */
#define  TRX_RX_ADDR_P5         XBYTE[0x14]  /* RX_ADDR_P5 (仅LSB, MSB部分与P1相同) */

/* 发送地址寄存器 (5字节) */
#define  TRX_TX_ADDR_0          XBYTE[0x15]  /* TX_ADDR 字节0 (LSB) */
#define  TRX_TX_ADDR_1          XBYTE[0x16]  /* TX_ADDR 字节1 */
#define  TRX_TX_ADDR_2          XBYTE[0x17]  /* TX_ADDR 字节2 */
#define  TRX_TX_ADDR_3          XBYTE[0x18]  /* TX_ADDR 字节3 */
#define  TRX_TX_ADDR_4          XBYTE[0x19]  /* TX_ADDR 字节4 (MSB) */

/* ============== 有效载荷宽度寄存器 ============== */
#define  TRX_RX_PW_P0           XBYTE[0x1a]  /* 管道0接收有效载荷宽度 (0-32字节, 0=禁用) */
#define  TRX_RX_PW_P1           XBYTE[0x1b]  /* 管道1接收有效载荷宽度 (0-32字节, 0=禁用) */
#define  TRX_RX_PW_P2           XBYTE[0x1c]  /* 管道2接收有效载荷宽度 (0-32字节, 0=禁用) */
#define  TRX_RX_PW_P3           XBYTE[0x1d]  /* 管道3接收有效载荷宽度 (0-32字节, 0=禁用) */
#define  TRX_RX_PW_P4           XBYTE[0x1e]  /* 管道4接收有效载荷宽度 (0-32字节, 0=禁用) */
#define  TRX_RX_PW_P5           XBYTE[0x1f]  /* 管道5接收有效载荷宽度 (0-32字节, 0=禁用) */

/* ============== 动态有效载荷和特性寄存器 ============== */
#define  TRX_DYNPD              XBYTE[0x20]  /* 动态有效载荷使能寄存器 */
/* 位定义:
 * bit[5:0]: DPL_P5~DPL_P0 - 分别使能6个管道的动态有效载荷长度
 */

#define  TRX_FEATURE            XBYTE[0x21]  /* 特性使能寄存器 */
/* 位定义:
 * bit[2]: EN_DPL - 启用动态有效载荷长度，接收pipes能解析动态长度包
 * bit[1]: EN_ACK_PAY - 允许接收方在ACK包中附加数据(附加数据使用TRX_CMD=0x68~0x6D命令写入)，依赖TRX_EN_AA寄存器
 * bit[0]: EN_DYN_ACK - 允许发送无需ACK确认的数据包(W_TX_PAYLOAD_NOACK命令)
 */

/* ============== 配置寄存器扩展 ============== */
#define  TRX_CFG_0C_0           XBYTE[0x22]  /* 配置寄存器0C字节0 */
#define  TRX_CFG_0C_1           XBYTE[0x23]  /* 配置寄存器0C字节1 */
#define  TRX_CFG_0C_2           XBYTE[0x24]  /* 配置寄存器0C字节2 */
#define  TRX_CFG_0C_3           XBYTE[0x25]  /* 配置寄存器0C字节3 */
#define  TRX_CFG_0D_0           XBYTE[0x26]  /* 配置寄存器0D字节0 */
#define  TRX_CFG_0D_1           XBYTE[0x27]  /* 配置寄存器0D字节1 */
#define  TRX_CFG_0D_2           XBYTE[0x28]  /* 配置寄存器0D字节2 */
#define  TRX_CFG_0D_3           XBYTE[0x29]  /* 配置寄存器0D字节3 */

/* ============== RAMP表寄存器 ============== */
#define  TRX_RAMP_TABLE_0       XBYTE[0x2a]  /* RAMP表寄存器0 */
#define  TRX_RAMP_TABLE_1       XBYTE[0x2b]  /* RAMP表寄存器1 */
#define  TRX_RAMP_TABLE_2       XBYTE[0x2c]  /* RAMP表寄存器2 */
#define  TRX_RAMP_TABLE_3       XBYTE[0x2d]  /* RAMP表寄存器3 */
#define  TRX_RAMP_TABLE_4       XBYTE[0x2e]  /* RAMP表寄存器4 */
#define  TRX_RAMP_TABLE_5       XBYTE[0x2f]  /* RAMP表寄存器5 */
#define  TRX_RAMP_TABLE_6       XBYTE[0x30]  /* RAMP表寄存器6 */
#define  TRX_RAMP_TABLE_7       XBYTE[0x31]  /* RAMP表寄存器7 */
#define  TRX_RAMP_TABLE_8       XBYTE[0x32]  /* RAMP表寄存器8 */
#define  TRX_RAMP_TABLE_9       XBYTE[0x33]  /* RAMP表寄存器9 */
#define  TRX_RAMP_TABLE_A       XBYTE[0x34]  /* RAMP表寄存器A */

/* ============== 控制与状态寄存器 ============== */
#define  TRX_CE                 XBYTE[0x35]  /* 芯片使能控制寄存器 */
/* 位定义:
 * bit[0]: BK2423_ce - 芯片使能控制
 */

#define  TRX_CMD                XBYTE[0x36]  /* 命令寄存器 */
/* 命令列表:
 * 0x80: FLUSH_RX - 清空RX FIFO
 * 0xA0: FLUSH_TX - 清空TX FIFO
 * 0x10: REUSE_TX_PL - 重用TX有效载荷
 * 0x40: READ_RX_PAYLOAD - 读RX有效载荷
 * 0x60: WRITE_TX_PAYLOAD - 写TX有效载荷
 * 0x68~0x6D: W_ACK_PAYLOAD (管道0-5) - 写ACK有效载荷
 * 0x68: W_TX_PAYLOAD_NOACK - 写TX有效载荷(不期待ACK)
 * 0x00: NOP - 空操作
 */

#define  TRX_FIFO               XBYTE[0x37]  /* FIFO数据寄存器 */
/* 说明:
 * TX模式: 发送数据有效载荷寄存器 (1-32字节)
 * RX模式: 接收数据有效载荷寄存器 (1-32字节)
 */

/* ============== 状态寄存器 ============== */
#define  TRX_IRQ_STATUS         XBYTE[0x40]  /* 中断状态寄存器 */
/* 中断状态位定义 */
#define  B_IRQ_RX_DR            (1<<6)  /* bit[6]: 接收数据就绪中断 */
#define  B_IRQ_TX_DS            (1<<5)  /* bit[5]: 发送完成中断 */
#define  B_IRQ_MAX_RT           (1<<4)  /* bit[4]: 最大重传次数中断 */
#define  B_IRQ_TX_FULL          (1<<0)  /* bit[0]: TX FIFO满中断 */

/* 自己添加的 */
#define  B_RX_P_NO              (1<<1|1<<2|1<<3)  /* bit[3:1]: 指示当前FIFO的数据来自哪个管道 */
/* 说明:
 * RX_DR: 当新数据到达RX FIFO时置位
 * TX_DS: 当TX数据发送完成时置位 (如果启用了AUTO_ACK，则仅在收到ACK时置位)
 * MAX_RT: 达到最大重传次数时置位
 * TX_FULL: TX FIFO满标志
 */

#define  TRX_OBSERVE_TX         XBYTE[0x41]  /* TX观察寄存器 */
#define  TRX_RSSI_IND           XBYTE[0x42]  /* RSSI指示寄存器 */
/* 位定义:
 * bit[0]: clr_cd - 载波检测清除
 */

#define  TRX_FIFO_STATUS        XBYTE[0x43]  /* FIFO状态寄存器 */
/* FIFO状态位定义 */
#define  B_FIFO_TX_REUSE        (1<<6)  /* bit[6]: TX数据重用标志 */
#define  B_FIFO_TX_FULL         (1<<5)  /* bit[5]: TX FIFO满标志 */
#define  B_FIFO_TX_EMPTY        (1<<4)  /* bit[4]: TX FIFO空标志 */
#define  B_FIFO_RX_FULL         (1<<1)  /* bit[1]: RX FIFO满标志 */
#define  B_FIFO_RX_EMPTY        (1<<0)  /* bit[0]: RX FIFO空标志 */
#define  TRX_RX_RPL_WIDTH       XBYTE[0x44]  /* RX有效载荷宽度寄存器 */
/* 位定义:
 * bit[5:0]: BK2423_rpl_width - 有效载荷宽度
 */

#define  TRX_MBIST_STATUS       XBYTE[0x45]  /* MBIST状态寄存器 */
/* 位定义:
 * bit[2]: mbist_done - MBIST完成标志
 * bit[1]: mbist_pass - MBIST通过标志
 * bit[0]: mbist_fail - MBIST失败标志
 */

/* ============== 命令宏定义 ============== */
/* 清空TX FIFO */
#define  FLUSH_TX               { TRX_CMD = 0xA0; }
/* 清空RX FIFO */
#define  FLUSH_RX               { TRX_CMD = 0x80; }
/* 重用TX有效载荷 */
#define  REUSE_TX_PL            { TRX_CMD = 0x10; }

/* ============== 常量定义 ============== */
#define  TIMES_FOR_LOOP_PIPES   20    /* 管道循环检测次数 */
#define  MAX_PIPE_NUMBER        6     /* 最大管道数量 (0-5) */
#define  RX_PLOAD_MAX_WIDTH     0x20  /* 最大接收有效载荷宽度 (32字节) */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __DRIVER_BK24_H__ */
