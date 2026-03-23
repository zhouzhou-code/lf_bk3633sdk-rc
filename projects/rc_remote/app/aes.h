#ifndef AES_H_
#define AES_H_
#include <stdint.h>
#include <string.h>            // string functions
#include "arch.h"

/// End of the common EM part

#define AES_ENCODE      0
#define AES_DECODE      1

/// define the force inlining attribute for this compiler
#define __INLINE __forceinline static
#define EM_ENC_IN_OFFSET 0x130
#define EM_ENC_OUT_OFFSET 0x140

#define REG_EM_ET_BASE_ADDR   0x00814000
#define ip_AESKEY31_0_ADDR    0x008200B4
#define ip_AESKEY63_32_ADDR   0x008200B8
#define ip_AESKEY95_64_ADDR   0x008200BC
#define ip_AESKEY127_96_ADDR  0x008200C0
#define ip_AESPTR_ADDR        0x008200C4
#define ip_INTACK1_ADDR       0x00820020
#define ip_INTCNTL1_ADDR      0x00820018
#define ip_AESCNTL_ADDR       0x008200B0
#define ip_INTSTAT1_ADDR      0x0082001C

#define IP_CRYPTINTSTAT_BIT            ((uint32_t)0x00000004)
#define IP_SLPINTSTAT_BIT              ((uint32_t)0x00000002)


/// Exchange memory base address
#define EM_BASE_ADDR         REG_EM_ET_BASE_ADDR

/// Macro to write a common ip register
#define REG_IP_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)
/// Macro to read a common ip register
#define REG_IP_RD(addr)              (*(volatile uint32_t *)(addr))

/// Write bytes to EM
__INLINE void em_wr(void const *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy((void *)(em_addr + EM_BASE_ADDR), sys_addr, len);
}

/**

*******************************************************************************
*********
 * @brief Read a packed 16 bits word.
 * @param[in] ptr16 The address of the first byte of the 16 bits word.
 * @return The 16 bits value.

*******************************************************************************
*********
 */
__INLINE uint16_t co_read16p(void const *ptr16)
{
    uint16_t value = ((uint8_t *)ptr16)[0] | ((uint8_t *)ptr16)[1] << 8;
    return value;
}


/**

*******************************************************************************
*********
 * @brief Read a packed 32 bits word.
 * @param[in] ptr32 The address of the first byte of the 32 bits word.
 * @return The 32 bits value.

*******************************************************************************
*********
 */
__INLINE uint32_t co_read32p(void const *ptr32)
{
    uint16_t addr_l, addr_h;
    addr_l = co_read16p(ptr32);
    addr_h = co_read16p((uint8_t *)ptr32 + 2);
    return ((uint32_t)addr_l | (uint32_t)addr_h << 16);
}

__INLINE void ip_aeskey31_0_set(uint32_t value)
{
    REG_IP_WR(ip_AESKEY31_0_ADDR, value);
}

__INLINE void ip_aeskey63_32_set(uint32_t value)
{
    REG_IP_WR(ip_AESKEY63_32_ADDR, value);
}

__INLINE void ip_aeskey95_64_set(uint32_t value)
{
    REG_IP_WR(ip_AESKEY95_64_ADDR, value);
}

__INLINE void ip_aeskey127_96_set(uint32_t value)
{
    REG_IP_WR(ip_AESKEY127_96_ADDR, value);
}

__INLINE void ip_aesptr_setf(uint16_t aesptr)
{
    ASSERT_ERR((((uint32_t)aesptr << 0) & ~((uint32_t)0x00003FFF)) == 0);
    REG_IP_WR(ip_AESPTR_ADDR, (uint32_t)aesptr << 0);
}

__INLINE void ip_intack1_cryptintack_clearf(uint8_t cryptintack)
{
    ASSERT_ERR((((uint32_t)cryptintack << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_IP_WR(ip_INTACK1_ADDR, (uint32_t)cryptintack << 2);
}

__INLINE void ip_intack1_slpintack_clearf(uint8_t slpintack)
{
    ASSERT_ERR((((uint32_t)slpintack << 1) & ~((uint32_t)0x00000002)) == 0);
    REG_IP_WR(ip_INTACK1_ADDR, (uint32_t)slpintack << 1);
}

__INLINE void ip_intcntl1_cryptintmsk_setf(uint8_t cryptintmsk)
{
    ASSERT_ERR((((uint32_t)cryptintmsk << 2) & ~((uint32_t)0x00000004)) == 0);
    REG_IP_WR(ip_INTCNTL1_ADDR, (REG_IP_RD(ip_INTCNTL1_ADDR) & ~((uint32_t)0x00000004)) | ((uint32_t)cryptintmsk << 2));
}

__INLINE void ip_aescntl_aes_start_setf(uint8_t aesstart)
{
    ASSERT_ERR((((uint32_t)aesstart << 0) & ~((uint32_t)0x00000001)) == 0);
    REG_IP_WR(ip_AESCNTL_ADDR, (REG_IP_RD(ip_AESCNTL_ADDR) & ~((uint32_t)0x00000001)) | ((uint32_t)aesstart << 0));
}

__INLINE void em_rd(void *sys_addr, uint16_t em_addr, uint16_t len)
{
    memcpy(sys_addr, (void *)(em_addr + EM_BASE_ADDR), len);
}

__INLINE uint32_t ip_intstat1_get(void)
{
    return REG_IP_RD(ip_INTSTAT1_ADDR);
}

__INLINE void ip_aescntl_aes_mode_setf(uint8_t aesmode)
{
    REG_IP_WR(ip_AESCNTL_ADDR, (REG_IP_RD(ip_AESCNTL_ADDR) & ~((uint32_t)0x00000002)) | ((uint32_t)aesmode << 1));
}


extern void app_aes_test(uint8_t *key, uint8_t *value,uint8_t *encrypted);
#endif
