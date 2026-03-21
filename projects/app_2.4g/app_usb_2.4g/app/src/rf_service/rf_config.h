/**
 ****************************************************************************************
 * @file rf_config.h
 * @brief RF配置管理 - 极简Flash存储方案
 ****************************************************************************************
 */
#ifndef _RF_CONFIG_H_
#define _RF_CONFIG_H_

#include <stdint.h>

#define RF_ADDR_LEN 5

/* 设备类型枚举 (0-3, 适配协议INFO字段2bit位域) */
typedef enum {
    DEV_TYPE_REMOTE     = 0,    /* 遥控器 */
    DEV_TYPE_ESC        = 1,    /* 电控 */
    DEV_TYPE_BATTERY    = 2,    /* 动力电池 */
    DEV_TYPE_CADENCE    = 3,    /* 踏频 */
} device_type_t;

/* Flash 存储结构 - 极简版 */
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;                    // 0x5AA5BEEF
    uint8_t  esc_addr[RF_ADDR_LEN];    // 电控地址
    uint8_t  battery_addr[RF_ADDR_LEN];// 动力电池地址
    uint8_t  cadence_addr[RF_ADDR_LEN];// 踏频地址
    uint8_t  valid_mask;               // bit0=电控, bit1=动力电池, bit2=踏频
    uint8_t  checksum;
} rf_config_t;
#pragma pack(pop)

#define FLASH_RF_CONFIG_ADDR  0x7E000
#define FLASH_MAGIC_NUM       0x5AA5BEEF

/* 默认配对地址 */
#define RF_PAIR_ADDR_DEFAULT  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}

/**
 * @brief 从Flash加载配置到内存缓存
 */
void rf_config_load_from_flash(void);

/**
 * @brief 保存内存缓存到Flash
 */
void rf_config_save_to_flash(void);

/**
 * @brief 更新内存缓存中的设备地址（不写Flash）
 * @param dev 设备类型
 * @param addr 5字节地址
 */
void rf_config_update_device_addr(device_type_t dev, const uint8_t *addr);

/**
 * @brief 从内存缓存读取设备地址
 * @param dev 设备类型
 * @param addr [输出] 5字节地址
 * @return 1=有效, 0=未配对
 */
uint8_t rf_config_read_device_addr(device_type_t dev, uint8_t *addr);

/**
 * @brief 获取默认配对地址
 * @param addr [输出] 5字节地址
 */
void rf_config_get_pair_addr(uint8_t *addr);

/**
 * @brief 清除所有配对信息
 */
void rf_config_clear_all(void);

/**
 * @brief 生成随机地址（5字节）
 * @param out_addr [输出] 5字节地址
 */
void rf_config_generate_addr(uint8_t *out_addr);

#endif /* _RF_CONFIG_H_ */
