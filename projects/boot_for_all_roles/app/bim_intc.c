/**
 ****************************************************************************************
 *
 * @file bim_intc.c
 *
 * @brief Definition of the Interrupt Controller (INTCTRL) API.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */
#include "bim_intc.h"
#include <string.h>
#include "bim_uart2.h"
#include "bim_wdt.h"
#include "bim_icu.h"
#include "bim_adc.h"
#include "bim_uart0.h"


typedef void (*FUNCPTR_T)(void);

void printf_arm_reg()
{
#if 0
    unsigned int spReg, lrReg, pcReg;
    #pragma arm
    __asm
    {
        MOV spReg, __current_sp()
        MOV pcReg, __current_pc()
        MOV lrReg, __return_address()
    }

    bim_printf("SP = 0x%X\n",spReg);
    bim_printf("PC = 0x%X\n",pcReg);
    bim_printf("LR = 0x%X\n",lrReg);
#else
    unsigned int i,r;

    for(i=0;i<13;i++)
    {
        r=*((volatile unsigned long *)(0x400004+4*i));
        bim_printf("r=%x\r\n",r);
    }
    unsigned int sp=*((volatile unsigned long *)(0x400004+4*13));
    bim_printf("sp=%x\r\n",sp);

    unsigned int lr=*((volatile unsigned long *)(0x400004+4*14));
    bim_printf("lr=%x\r\n",lr);

    unsigned int pc=*((volatile unsigned long *)(0x400004+4*15));
    bim_printf("pc=%x\r\n",pc);
    
#endif
}




void Undefined_Exception(void)
{
    printf_arm_reg();
    bim_printf("Undefined_Exception\r\n");
    
    wdt_enable(0X100);
    
}
void SoftwareInterrupt_Exception(void)
{
    printf_arm_reg();
    bim_printf("SoftwareInterrupt_Exception\r\n");
    
    wdt_enable(0X100);
}
void PrefetchAbort_Exception(void)
{
    printf_arm_reg();
    bim_printf("PrefetchAbort_Exception\r\n");
    
    wdt_enable(0X100);
}
void DataAbort_Exception(void)
{
    printf_arm_reg();
    bim_printf("DataAbort_Exception\r\n");
    
    wdt_enable(0X100);
}

void Reserved_Exception(void)
{
    printf_arm_reg();
    bim_printf("Reserved_Exception\r\n");
    
    wdt_enable(0X100);
}

extern void adc_isr(void);


#if defined(__CC_ARM)
#pragma ARM
__irq 
#endif
void Irq_Exception(void)
{
    uint32_t IntStat=SYS_REG0X12;
    //bim_printf("irq int\n");
    #ifdef CHECK_LOW_VOLT_ENABLE
    if(IntStat & (1<<POS_SYS_REG0X12_INT_ADC_STA))
    {
        //bim_printf("adc int\n");
        adc_isr();
    }
    #endif
    SYS_REG0X12=IntStat;

}
