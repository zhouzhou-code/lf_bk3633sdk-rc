/**
****************************************************************************************
*
* @file icu.c
*
* @brief icu initialization and specific functions
*
* Copyright (C) Beken 2009-2016
*
* $Rev: $
*
****************************************************************************************
*/
#include <stddef.h>     // standard definition
#include "bim_icu.h"      // timer definition
#include "bim_uart2.h"
#include "bk3633_reglist.h"

void icu_init(void)
{
    SYS_REG0X0 &= ~(1<<POS_SYS_REG0X0_JTAG_MODE);///close JTAG
    SYS_REG0X2 =0;
    SYS_REG0X2 = (1<< POS_SYS_REG0X2_CORE_SEL);///16M CLK
    SYS_REG0X17 &= ~0x2;
    SYS_REG0X0D |= 0x40;

}
void cpu_reset(void)
{
    setf_PMU_Reg0x1_reg0_w_en;
    setf_PMU_Reg0x0_all_reset;
}
extern volatile uint32_t XVR_ANALOG_REG_BAK[32];
extern void Delay_ms(int num); //sync from svn revision 18

void mcu_clk_switch(uint8_t clk)
{
    switch(clk)
    {
        case MCU_CLK_16M:
        {
            set_SYS_Reg0x2_core_div(0x1);
            set_SYS_Reg0x2_core_sel(0x01);

        }break;
        case MCU_CLK_32M:
        {
            ///cpu 64M,spi 64M那㊣?車?∩
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 20);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 17);
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 16);
            addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
            Delay_ms(1);
            clrf_SYS_Reg0x17_CLK96M_PWD;
            set_SYS_Reg0x2_core_div(0x2);
            set_SYS_Reg0x2_core_sel(0x03);

        }break;
        case MCU_CLK_64M:
        {
            ///cpu 64M,spi 64M那㊣?車?∩
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 20);
            XVR_ANALOG_REG_BAK[9]&= ~(0x01 << 17);
            XVR_ANALOG_REG_BAK[9]|= (0x01 << 16);
            addXVR_Reg0x9 = XVR_ANALOG_REG_BAK[9];
            Delay_ms(1);
            clrf_SYS_Reg0x17_CLK96M_PWD;
            set_SYS_Reg0x2_core_div(0x1);
            set_SYS_Reg0x2_core_sel(0x03);

        }break;
        default:break;
    }

}

