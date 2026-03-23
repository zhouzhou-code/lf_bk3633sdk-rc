/**
 ****************************************************************************************
 * @file rf_config.c
 * @brief RF配置管理 - 极简Flash存储实现 + 地址生成
 ****************************************************************************************
 */
#include "rf_config.h"
#include "flash.h"
#include "user_config.h"
#include "timer_handler.h"
#include "bk3633_reglist.h"
#include "ll.h"
#include <string.h>

/* Flash 操作相关宏 */
#define OP_IDX_PP             12   // Page Program 索引
#define OP_IDX_SE             13   // Sector Erase 索引
#define BIT_ADDRESS_SW        0
#define BIT_OP_TYPE_SW        24
#define BIT_OP_SW             29

/* 全局配置缓存,上电时先把设备地址读取到这里 */
static rf_config_t g_rf_config;

/* 默认配对地址 */
static const uint8_t s_pair_addr_default[RF_ADDR_LEN] = RF_PAIR_ADDR_DEFAULT;

/* 地址池：用于生成随机地址 */
typedef struct {
    uint8_t bitmap[32]; // 256位占用位图
} addr_pool_t;

static addr_pool_t g_addr_pool;
/**
 * @brief 初始化地址池
 */
static void addr_pool_init(void)
{
    memset(&g_addr_pool, 0, sizeof(g_addr_pool));
}

/**
 * @brief 生成随机字节（带冲突检测）
 */
static uint8_t gen_random_byte(void)
{
    static uint8_t entropy_counter = 0;
    entropy_counter++;

    uint32_t seed = entropy_counter + Get_SysTick_ms();
    uint8_t start_index = (uint8_t)((seed * 131) ^ (seed >> 8));
    const uint8_t STEP = 31;

    for (int i = 0; i < 256; i++) {
        uint8_t curr_id = (start_index + (i * STEP)) % 256;
        uint8_t byte_idx = curr_id / 8;
        uint8_t bit_idx = curr_id % 8;

        if (!(g_addr_pool.bitmap[byte_idx] & (1 << bit_idx))) {
            g_addr_pool.bitmap[byte_idx] |= (1 << bit_idx);
            return curr_id;
        }
    }
    return 0; // 池满，返回0
}

/**
 * @brief 计算校验和
 */
static uint8_t calc_checksum(const rf_config_t *cfg)
{
    uint8_t sum = 0;
    const uint8_t *p = (const uint8_t *)cfg;
    for (uint32_t i = 0; i < sizeof(rf_config_t) - 1; i++) {
        sum += p[i];
    }
    return sum;
}

/**
 * @brief 强制解锁 Flash 写保护
 */
static void flash_force_unlock(void)
{
    flash_write_sr(1, 0x80);
}

/**
 * @brief 强制擦除指定地址所在扇区
 */
static void flash_erase_force(uint32_t address)
{
    flash_force_unlock();
    GLOBAL_INT_DISABLE();
    while(REG_FLASH_OPERATE_SW & 0x80000000);

    REG_FLASH_OPERATE_SW = ((address << BIT_ADDRESS_SW)
                          | (OP_IDX_SE << BIT_OP_TYPE_SW)
                          | (0x1 << BIT_OP_SW));
    while(REG_FLASH_OPERATE_SW & 0x80000000);

    flash_wp_all();  // 恢复写保护
    GLOBAL_INT_RESTORE();
}

/**
 * @brief Flash 写入用户数据（32字节对齐）
 */
static void flash_write_user_data(uint32_t address, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint32_t addr = address & (~0x1F);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];

    if (len == 0) return;

    flash_force_unlock();
    GLOBAL_INT_DISABLE();

    while(len)
    {
        if((address & 0x1F) || (len < 32)) {
            flash_read_data(pb, addr, 32);
        } else {
            memset(buf, 0xFF, 32);
        }

        for(i = (address & 0x1F); i < 32; i++) {
            if(len) {
                pb[i] = *data++;
                address++;
                len--;
            }
        }

        while(REG_FLASH_OPERATE_SW & 0x80000000);
        for (i=0; i<8; i++) REG_FLASH_DATA_SW_FLASH = buf[i];

        REG_FLASH_OPERATE_SW = ((addr << BIT_ADDRESS_SW)
                              | (OP_IDX_PP << BIT_OP_TYPE_SW)
                              | (0x1 << BIT_OP_SW));

        while(REG_FLASH_OPERATE_SW & 0x80000000);
        addr += 32;
    }
    flash_wp_all();  // 恢复写保护
    GLOBAL_INT_RESTORE();
}

/**
 * @brief 从Flash加载配置到内存缓存
 */
void rf_config_load_from_flash(void)
{
    /* 读取Flash */
    flash_read(0, FLASH_RF_CONFIG_ADDR, sizeof(rf_config_t), (uint8_t *)&g_rf_config, NULL);

    /* 调试：打印读取的数据 */
    uart_printf("Flash Read: magic=0x%08lX, valid_mask=0x%02X, checksum=0x%02X\r\n",
                g_rf_config.magic, g_rf_config.valid_mask, g_rf_config.checksum);
    uint8_t calc_sum = calc_checksum(&g_rf_config);
    uart_printf("Calculated checksum=0x%02X\r\n", calc_sum);

    /* 校验魔数和校验和 */
    if (g_rf_config.magic != FLASH_MAGIC_NUM ||
        g_rf_config.checksum != calc_checksum(&g_rf_config)) {
        /* 无效数据，初始化为默认值 */
        uart_printf("RF Config: Invalid, use default\r\n");
        memset(&g_rf_config, 0, sizeof(rf_config_t));
        g_rf_config.magic = FLASH_MAGIC_NUM;
        g_rf_config.valid_mask = 0;
        g_rf_config.checksum = calc_checksum(&g_rf_config);
    } else {
        uart_printf("RF Config: Loaded from Flash\r\n");
        uart_printf("  ESC: %02X:%02X:%02X:%02X:%02X\r\n", g_rf_config.esc_addr[0], g_rf_config.esc_addr[1], g_rf_config.esc_addr[2], g_rf_config.esc_addr[3], g_rf_config.esc_addr[4]);
        uart_printf("  BATTERY: %02X:%02X:%02X:%02X:%02X\r\n", g_rf_config.battery_addr[0], g_rf_config.battery_addr[1], g_rf_config.battery_addr[2], g_rf_config.battery_addr[3], g_rf_config.battery_addr[4]);
        uart_printf("  CADENCE: %02X:%02X:%02X:%02X:%02X\r\n", g_rf_config.cadence_addr[0], g_rf_config.cadence_addr[1], g_rf_config.cadence_addr[2], g_rf_config.cadence_addr[3], g_rf_config.cadence_addr[4]);
    }
}

/**
 * @brief 保存内存缓存到Flash
 */
void rf_config_save_to_flash(void)
{
    /* 更新校验和 */
    g_rf_config.checksum = calc_checksum(&g_rf_config);

    /* 调试：打印保存的数据 */
    uart_printf("Flash Save: magic=0x%08lX, valid_mask=0x%02X, checksum=0x%02X\r\n",
                g_rf_config.magic, g_rf_config.valid_mask, g_rf_config.checksum);

    /* 擦除扇区 */
    flash_erase_force(FLASH_RF_CONFIG_ADDR);

    /* 写入数据 */
    flash_write_user_data(FLASH_RF_CONFIG_ADDR, (uint8_t *)&g_rf_config, sizeof(rf_config_t));

    uart_printf("RF Config: Saved to Flash\r\n");
}

/**
 * @brief 更新内存缓存中的设备地址（不写Flash）
 */
void rf_config_update_device_addr(device_type_t dev, const uint8_t *addr)
{
    uint8_t *target = NULL;
    uint8_t bit_mask = 0;

    /* 确保magic有效 */
    if (g_rf_config.magic != FLASH_MAGIC_NUM) {
        g_rf_config.magic = FLASH_MAGIC_NUM;
    }

    switch (dev) {
    case DEV_TYPE_ESC:
        target = g_rf_config.esc_addr;
        bit_mask = (1 << 0);
        break;
    case DEV_TYPE_BATTERY:
        target = g_rf_config.battery_addr;
        bit_mask = (1 << 1);
        break;
    case DEV_TYPE_CADENCE:
        target = g_rf_config.cadence_addr;
        bit_mask = (1 << 2);
        break;
    default:
        return;
    }

    memcpy(target, addr, RF_ADDR_LEN);
    g_rf_config.valid_mask |= bit_mask;
}

/**
 * @brief 从内存缓存读取设备地址
 */
uint8_t rf_config_read_device_addr(device_type_t dev, uint8_t *addr)
{
    const uint8_t *source = NULL;
    uint8_t bit_mask = 0;

    switch (dev) {
    case DEV_TYPE_ESC:
        source = g_rf_config.esc_addr;
        bit_mask = (1 << 0);
        break;
    case DEV_TYPE_BATTERY:
        source = g_rf_config.battery_addr;
        bit_mask = (1 << 1);
        break;
    case DEV_TYPE_CADENCE:
        source = g_rf_config.cadence_addr;
        bit_mask = (1 << 2);
        break;
    default:
        return 0;
    }

    if (g_rf_config.valid_mask & bit_mask) {
        memcpy(addr, source, RF_ADDR_LEN);
        return 1;
    }
    return 0;
}

/**
 * @brief 获取默认配对地址
 */
void rf_config_get_pair_addr(uint8_t *addr)
{
    memcpy(addr, s_pair_addr_default, RF_ADDR_LEN);
}

/**
 * @brief 清除所有配对信息
 */
void rf_config_clear_all(void)
{
    memset(&g_rf_config, 0, sizeof(rf_config_t));
    g_rf_config.magic = FLASH_MAGIC_NUM;
    g_rf_config.valid_mask = 0;
    rf_config_save_to_flash();
    uart_printf("RF Config: Cleared\r\n");
}

/**
 * @brief 生成随机地址（5字节）
 * @param out_addr [输出] 5字节地址
 */
void rf_config_generate_addr(uint8_t *out_addr)
{
    if (!out_addr) return;

    addr_pool_init();
    for (int i = 0; i < RF_ADDR_LEN; i++) {
        out_addr[i] = gen_random_byte();
    }
}
