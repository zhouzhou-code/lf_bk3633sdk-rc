#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include <stddef.h>     // standard definition
#include "BK3633_RegList.h"
#include "bim_adc.h"
#include "bim_wdt.h"
#include "bim_icu.h"
#include "bim_uart2.h"


void Delay_us(int num)
{
    volatile int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 10; x++);
    }
}

void Delay_ms(int num)
{
    volatile int x, y;
    for(y = 0; y < num; y ++ )
    {
        for(x = 0; x < 3000; x++);
    }
}

#ifdef CHECK_LOW_VOLT_ENABLE
uint16_t g_adc_value;
uint8_t adc_flag;
uint16_t referance_voltage;
volatile uint32_t XVR_ANALOG_REG_BAK_07 = 0;
/************************************************************************
//ADC参考电压默认为1.05V
//确保ADC稳定采样，ADC口需要接个10nf到地的电容
*************************************************************************/
void adc_init(uint8_t channel,uint8_t mode)
{
    uint32_t cfg;

    //addSYS_Reg0x10 = 0; // int enable 0:disable 1::enable
    //addSYS_Reg0x11 = 0; // priority; 0: irq  1:fiq

    //enable adc clk
    SET_SADC_POWER_UP;

    //set special as peripheral func,set float is  peripheral function,the 2th function id clockout
    //gpio_config(0x30 + channel,FLOAT,PULL_NONE); 

    //set adc mode/channel/wait clk
    cfg = ( (mode << POS_SADC_REG0X0_CFG0_MODE ) 
           | (channel << POS_SADC_REG0X0_CFG0_CHNL) 
           | (0x01 << POS_SADC_REG0X0_CFG0_SETING));

    cfg |= ((18 << POS_SADC_REG0X0_CFG0_SAMP_RATE) 
          | (5 << POS_SADC_REG0X0_CFG0_PRE_DIV)
          | (0x0 << POS_SADC_REG0X0_CFG0_ADC_FILTER)
          | (0x01 << POS_SADC_REG0X0_CFG0_INT_CLEAR));

    SADC_REG0X0_CFG0=cfg;
    //REG_APB7_ADC_CFG |= (0x01 << BIT_ADC_EN);//不能先使能ADC，不然ADC FIFO满时没有读出再次启动ADC就不会有中断

    SADC_REG0X2_CFG1 = ((1<<POS_SADC_REG0X2_CHANN_EXPAND)|(1<<POS_SADC_REG0X2_STEADY_CTRL));
    SADC_REG0X3_CFG2 = (3<<POS_SADC_REG0X3_STA_CTRL);

    SYS_REG0X10_INT_EN |= (0x01 << POS_SYS_REG0X10_INT_EN_ADC);

    if(mode==3)
        SADC_REG0X0_CFG0 |= SET_ADC_EN;
}

void adc_isr(void)
{
    SADC_REG0X0_CFG0 |= (0x01 << POS_SADC_REG0X0_CFG0_INT_CLEAR);
    adc_flag = 1;       
}

#define REG_AHB0_ICU_INT_FLAG (*(volatile uint32_t *)(0x00800048))
#define INT_STATUS_ADC_bit       (0x01<< 8)
uint16_t adc_get_value(uint8_t channel,uint8_t mode)
{   
    uint16_t adc_cnt;
    adc_cnt=0;
    adc_flag =0;

    if((SADC_REG0X0_CFG0&0x03)==0x03)
        return 0;
    
    SADC_REG0X0_CFG0 |= SET_ADC_EN+(mode << POS_SADC_REG0X0_CFG0_MODE )+(channel << POS_SADC_REG0X0_CFG0_CHNL);
    
    uint32_t IntStat = REG_AHB0_ICU_INT_FLAG;

    while (!adc_flag)  
    {
        adc_cnt++;       
        if(adc_cnt>3000)
        {
            //uart_printf("g_adc_value_timeout\r\n");
            bim_printf("g_adc_value_timeout\r\n");
            break;            
        }
        Delay_us(10);
    } 
    if(adc_flag==1)
    {
        g_adc_value=SADC_REG0X4_DAT>>2;
        //bim_printf("g_adc_value=%x,channel=%x\r\n",g_adc_value,channel);
    }
    
    SADC_REG0X0_CFG0 &= ~(SET_ADC_EN+(0x03 << POS_SADC_REG0X0_CFG0_MODE )+(0x0f << POS_SADC_REG0X0_CFG0_CHNL)); //ADC值读取完成后必须把使能位清除       
    return g_adc_value;     
}


#define LOW_VOlTAGE         2000 /// 2xxx mV
#define CHECK_VOLT_COUNT    3
uint8_t check_low_volt_sleep(void)
{
    uint8_t i;
    uint16_t referance_voltage = 0xffff;
    uint32_t calib_temp=0;
    XVR_ANALOG_REG_BAK_07 = 0xAA023FC0;
    XVR_ANALOG_REG_BAK_07 |= (1<<19);
    addXVR_Reg0x7 = XVR_ANALOG_REG_BAK_07;

    adc_init(7, 1);

    for(i = 0; i < CHECK_VOLT_COUNT; i++)
    {
        calib_temp += adc_get_value(7,1);
    }
    referance_voltage= (uint16_t)((calib_temp * 1000 / CHECK_VOLT_COUNT) / 256 * 1.05 * 4);

    bim_printf("vbat = %x mV\r\n", referance_voltage);

    XVR_ANALOG_REG_BAK_07 &= ~(1<<19);
    addXVR_Reg0x7 = XVR_ANALOG_REG_BAK_07;

    SYS_REG0X10_INT_EN &= ~(0x01 << POS_SYS_REG0X10_INT_EN_ADC);
    if(referance_voltage < LOW_VOlTAGE && referance_voltage != 0)
    {
        referance_voltage = 0;
        return 1;
    }
    referance_voltage =0;
    return 0;
}

#endif


