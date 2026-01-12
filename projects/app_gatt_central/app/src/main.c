/**
 ****************************************************************************************
 *
 * @file arch_main.c
 *
 * @brief Main loop of the application.
 *
 * Copyright (C) RivieraWaves 2009-2015
 *
 *
 ****************************************************************************************
 */
#include "rwip_config.h" // RW SW configuration
#include "arch.h"      // architectural platform definitions
#include <stdlib.h>    // standard lib functions
#include <stddef.h>    // standard definitions
#include <stdint.h>    // standard integer definition
#include <stdbool.h>   // boolean definition
#include <string.h>   // boolean definition
#include "rwip.h"      // RW SW initialization
#include "prf.h"      // RW SW initialization
#include "rwble.h"

#include "intc.h"      // Interrupt initialization
#if PLF_UART
#include "uart.h"      // UART initialization
#if PLF_UART2
#include "uart2.h"      // UART2 initialization
#endif // PLF_UART2
#endif //PLF_UART
#include "flash.h"     // Flash initialization
#include "led.h"       // Led initialization
#if (BLE_EMB_PRESENT || BT_EMB_PRESENT)
//#include "rf.h"        // RF initialization
#endif // BLE_EMB_PRESENT || BT_EMB_PRESENT

#if (BLE_APP_PRESENT)
#include "app.h"       // application functions
#endif

#if (NVDS_SUPPORT)
#include "nvds.h"         // NVDS definitions
#endif
#if(USB_DRIVER)
#include "driver_usb.h"
#include "usb.h"
#include "driver_icu.h"
#endif
#include "reg_access.h"
#include "boot.h"
#include "dbg.h"
#include "icu.h"
#include "user_config.h"
#include "co_utils.h" 
#include "gpio.h"
#include "icu.h"
#include "wdt.h"
#include "spi.h"
#include "adc.h"
#include "uart2.h"
#include "aon_rtc.h"
#include "lcd360g.h"
#include "oads.h"
#include "rf.h"
#include "pwm.h"
#include "sdp_service_task.h"
#include "sdp_service.h"
#include "app_init.h"
#if(TIMER0_DRIVER)
#include "timer0.h" 
#endif

static void stack_integrity_check(void)
{
    if ((REG_PL_RD(STACK_BASE_UNUSED)!= BOOT_PATTERN_UNUSED))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_UNUSED fail!\r\n");
        }
    }

    if ((REG_PL_RD(STACK_BASE_SVC)!= BOOT_PATTERN_SVC))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_SVC fail!\r\n");
        }
    }

    if ((REG_PL_RD(STACK_BASE_FIQ)!= BOOT_PATTERN_FIQ))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_FIQ fail!\r\n");
        }
    }

    if ((REG_PL_RD(STACK_BASE_IRQ)!= BOOT_PATTERN_IRQ))
    {
        while(1)
        {
            uart_printf("Stack_Integrity_Check STACK_BASE_IRQ fail!\r\n");
        }
    }

}

static void rom_env_init(struct rom_env_tag *api)
{
    memset(&rom_env,0,sizeof(struct rom_env_tag));
    rom_env.prf_get_id_from_task = prf_get_id_from_task;
    rom_env.prf_get_task_from_id = prf_get_task_from_id;
    rom_env.prf_init = prf_init;
    rom_env.prf_create = prf_create;
    rom_env.prf_cleanup = prf_cleanup;
    rom_env.prf_add_profile = prf_add_profile;
    
    rom_env.rwip_reset = rwip_reset;
    rom_env.platform_reset = platform_reset;
    rom_env.rwble_sleep_wakeup_end = rwble_sleep_wakeup_end;
    rom_env.stack_printf=uart2_printf;
    //rom_env.assert_err = assert_err;
    //rom_env.assert_param = assert_param;
    //rom_env.Read_Uart_Buf = Read_Uart_Buf;
    //rom_env.uart_clear_rxfifo = uart_clear_rxfifo;
    rom_env.setEvent = event_null_handle;
    rom_env.clearEvent = event_null_handle;
}


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
#if PLF_UART        ////Added
// Creation of uart external interface api
const struct rwip_eif_api uart_api =
{
    uart_read,
    uart_write,
    uart_flow_on,
    uart_flow_off,
};
#endif



void bdaddr_env_init(void)
{
    struct bd_addr co_bdaddr;
    uart_printf("bdaddr_def_addr_abs=%x\r\n",flash_env.bdaddr_def_addr_abs);
    flash_read_data(&co_bdaddr.addr[0],flash_env.bdaddr_def_addr_abs,6);

    if(co_bdaddr.addr[0]!=0xff ||co_bdaddr.addr[1]!=0xff||
    co_bdaddr.addr[2]!=0xff||co_bdaddr.addr[3]!=0xff||
    co_bdaddr.addr[4]!=0xff||co_bdaddr.addr[5]!=0xff )
    {
        memcpy(&co_default_bdaddr,&co_bdaddr,6);
    }
}

 

#if (PLF_DEBUG)
void assert_err(const char *condition, const char * file, int line)
{
    uart_printf("%s,condition %s,file %s,line = %d\r\n",__func__,condition,file,line);
}

void assert_param(int param0, int param1, const char * file, int line)
{
    uart_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
}

void assert_warn(int param0, int param1, const char * file, int line)
{
    uart_printf("%s,param0 = %d,param1 = %d,file = %s,line = %d\r\n",__func__,param0,param1,file,line);
}

void dump_data(uint8_t* data, uint16_t length)
{
    uart_printf("%s,data = %d,length = %d,file = %s,line = %d\r\n",__func__,data,length);
}

#endif //PLF_DEBUG



void platform_reset(uint32_t error)
{   
    uart_printf("reset error = %x\r\n", error);
    // Disable interrupts
    GLOBAL_INT_STOP();
    
    // Restart FW
    cpu_reset();    
}
       
void enter_dut_fcc_mode(void)
{
    while(1)
    {
        // schedule all pending events
        rwip_schedule();
        #if (VIRTUAL_UART_H4TL == 1)
        uart_h4tl_data_switch();
        #endif
    }
}
  
void enter_normal_app_mode(void)
{

    extern struct rev_ntf_data notify_data;
    //char ms_buf[4]={0};
    while(1)
    {
        // schedule all pending events
        rwip_schedule();

        if(notify_data.notify_media_key_status == 1)
        {
            notify_data.notify_media_key_status = 0;

            #if USB_DRIVER
            AplUsb_StartTx(USB_ENDPID_Hid_MKEY, notify_data.notify_media_key, 2);
            #endif
        }
        if(notify_data.notify_standard_key_status == 1)
        {
            notify_data.notify_standard_key_status= 0;

            #if USB_DRIVER
            notify_data.notify_standard_key[0]=0x02;//report id
            AplUsb_StartTx(USB_ENDPID_Hid_MSE, notify_data.notify_standard_key, 9);
            #endif
        }

        #if (!USB_DRIVER)  
        // Checks for sleep have to be done with interrupt disabled
        GLOBAL_INT_DISABLE();
        // Check if the processor clock can be gated

        if( NO_SLEEP!=icu_get_sleep_mode() )
        {
            switch(rwip_sleep())
            {
                case RWIP_DEEP_SLEEP:        
                {    
                    // add here platform specific deep sleep code  
                    if(MCU_REDUCE_VO_SLEEP==icu_get_sleep_mode())
                    {
                        cpu_reduce_voltage_sleep();         
                        cpu_wakeup();
                    }
                    else if(MCU_IDLE_SLEEP==icu_get_sleep_mode())
                    {
                        cpu_idle_sleep();
                    }
                    else if(MCU_DEEP_SLEEP==icu_get_sleep_mode())
                    {
                        ////before deepsleep use (deep_sleep_wakeup_set)  config gpio wakeup mcu
                        ////deep_sleep_wakeup_set(0X10);
                        deep_sleep();
                    }
                }
                break;
                case RWIP_CPU_SLEEP:
                {   
                    // Wait for interrupt
                    cpu_idle_sleep(); 
                }
                break;
                case RWIP_ACTIVE:
                default:
                {
                    // nothing to do.
                } break;
            }
        }
        // Checks for sleep have to be done with interrupt disabled
        GLOBAL_INT_RESTORE();
        #endif  
        stack_integrity_check();
    }
}


#if(USB_DRIVER)

extern void DelayNops_usb(volatile unsigned long nops);
void usb_mod_enable(int en_dis){
    if(en_dis){//usb modal enable
        setf_SYS_Reg0x3_usb_pwd;
        DelayNops_usb(500);
        clrf_SYS_Reg0x3_usb_pwd;
    }
    else{
        setf_SYS_Reg0x3_usb_pwd;
    }
}

void usb_mod_ie_enable(int en_dis){
    if(en_dis){
        ICU_INT_ENABLE_SET(ICU_INT_ENABLE_IRQ_USB_MASK);
    }else{
        ICU_INT_ENABLE_CLEAR(ICU_INT_ENABLE_IRQ_USB_MASK);
    }
}

int mouseTest=0;
char sendbuffer[2]={22,33};
void Hid_RxCbk(void*ptr,int sz){
    int i;
    char*dat=(char*)ptr;
    uart_printf("recieved a hid data = %x\r\n",sz);
    for(i=0;i<sz;i++)
        uart_printf("%x,",dat[i]);
    uart_printf("\r\n");
/*    if(((char*)ptr)[0]==0x10){
        uart_printf("recieved a hid data\r\n");
    }
    AplUsb_StartTx(USB_ENDPID_Hid_DBG_IN, ptr, sz);*/
//    print("Ep3 Rx Data",ptr,sz);
}
   
//volatile uint8_t b_isDataing = FALSE;
uint8_t user_flash_data1[16] = {0x12,0x34,0xaa,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
uint8_t write_prop_uuid[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB0, 0x00, 0x40, 0x51, 0x04,  0xC1, 0xFF,0x00, 0xF0};
void Ep2_RxCbk(void*ptr,int sz){
    uint8_t * pBuffer;
    pBuffer = (uint8_t*)ptr;
    
    uart_printf("dwLength: %d  \r\n",sz);
    for(int i=0;i<sz;i++)
    {
        uart_printf("%2x ",pBuffer[i]);
    }
    uart_printf("\r\n");
    //01 E0 FC 02 0E A5
    if((pBuffer[0] == 0x01) && (pBuffer[4] == 0x0e) &&(pBuffer[5] == 0xa5)   &&(pBuffer[1] == 0xe0)
    &&(pBuffer[2] == 0xfc) &&(pBuffer[3] == 0x02) )
    {
        flash_wp_256k();
        ///flash_erase(0,0x7ff00,16,NULL);
        flash_write(0,0x7d000,16,user_flash_data1,NULL);
        Delay_us(1000);
        #if 0      
        Delay_us(1000);
        memset(user_flash_data1,0x00,16);
        flash_read(0,0x7ff00,16,user_flash_data1,NULL);

        for(uint8_t i=0;i<16;i++)
        uart_printf("user_flash_data1[%x]=%x\n",i,user_flash_data1[i]);
        #endif                       
        platform_reset(0x1515);
    }
    else
    {
        uint16_t handle;
        handle=sdp_get_handle_from_uuid(0,16,write_prop_uuid);
        appc_write_service_data_req(0,handle,8,pBuffer);

    }
}

void MicIn_cbk(void*ptr,int sz)
{
    uint8_t src[32];

    if(1 == read_decode_data(src))
    {
        AplUsb_StartTx(USB_ENDPID_Audio_MIC,src,32);
    }
    else
    {
        memset(src,0x00,32);
        AplUsb_StartTx(USB_ENDPID_Audio_MIC,src, 32);
    }
}
void print(void*msg,void*dat,int len)
{
    char*ptr=(char*)dat;
    int i;
    uart_printf(msg);
    uart_printf("[%d]={\r\n\t",len);
    for(i=0;i<len;i++)
    {
        uart_printf("%.2x ",ptr[i]);
        if((i&0x07)==0x07)uart_printf("\r\n\t");
    }
    uart_printf("\r\n}\r\n");
}


void AudioOut_Cbk(void*ptr,int sz){
    static int cnt=0;
//     uart_send(&cnt,1);
//     uart_send(&sz,1);

    uart_printf("%dth %dB:\r\n",cnt&0xff,sz);
    //print("data",ptr,8);
    cnt++;
//    memset(ptr,0,sz);
}
#endif


int main(void)
{
    
    icu_init();
    
    wdt_disable();

    intc_init();
    
    #if(!USB_DRIVER)
    uart_init(115200);
    #else
    sys_mode_init(NORMAL_MODE);
    #endif
    uart2_init(1000000);//

    app_dir_read_notify_init();
    gpio_init(); 

    //timer_init();
    
    flash_init();

    bdaddr_env_init();

    srand(co_default_bdaddr.addr[0]+co_default_bdaddr.addr[5]);
     
#if (NVDS_SUPPORT)
    nvds_init();
#endif 
    rom_env_init(&rom_env);
    // Initialize RW SW stack
    rwip_init(0);
      
    uart_printf("rw_main  start-----------------------%x,%x \r\n",EM_BLE_END);

    if((system_mode & DUT_FCC_MODE) == DUT_FCC_MODE) //dut mode
    {
        uart_printf("enter_dut_fcc_mode \r\n");
        GLOBAL_INT_START();
        mcu_clk_switch(MCU_CLK_80M);
        enter_dut_fcc_mode();
    }
    else //normal mode
    {
        uart_printf("enter_normal_app_mode \r\n");
        icu_set_sleep_mode(MCU_REDUCE_VO_SLEEP);
        mcu_clk_switch(MCU_CLK_64M); 

       // void timer0_test_demo(void);
       //  timer0_test_demo();
        #if(AON_RTC_DRIVER)   
        aon_rtc_init();
        #endif
        #if(SPI_DRIVER)    
        spi_init(0,1,0);
        #endif

        #if(ADC_DRIVER) 
        adc_init(1,1);
        #endif

        #if(USB_DRIVER)
        usb_init(usb_mod_enable,usb_mod_ie_enable);
        AplUsb_SetRxCbk(USB_ENDPID_Hid_DBG_OUT, (void*)Ep2_RxCbk);
        AplUsb_SetTxCbk(USB_ENDPID_Audio_MIC,(void*)MicIn_cbk);
        AplUsb_SetRxCbk(USB_ENDPID_Audio_SPK, (void*)AudioOut_Cbk);
        AplUsb_SetRxCbk(USB_ENDPID_Hid_MSE_OUT, (void*)Hid_RxCbk);

        #endif
                 
        GLOBAL_INT_START();
     
        enter_normal_app_mode();
    }
    
}

const struct rwip_eif_api* rwip_eif_get(uint8_t idx)
{
    const struct rwip_eif_api* ret = NULL;
    switch(idx)
    {
        #if PLF_UART        ////
        case 0:
        {
            ret = &uart_api;
        }
        break;
        #endif
        default:
        {
            ASSERT_INFO(0, idx, 0);
        }
        break;
    }
    return ret;
}




