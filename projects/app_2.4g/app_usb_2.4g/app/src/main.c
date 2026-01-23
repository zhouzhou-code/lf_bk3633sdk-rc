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
 //#define __AES_TEST__
// #define __USB_TEST__
#include <stdlib.h>    // standard lib functions
#include <stddef.h>    // standard definitions
#include <stdint.h>    // standard integer definition
#include <stdbool.h>   // boolean definition
#include <string.h>   // boolean definition
#include "intc.h"      // Interrupt initialization
#include "uart.h"      // UART initialization
#include "uart2.h"      // UART2 initialization
#include "flash.h"     // Flash initialization
#include "app.h"       // application functions
#include "reg_access.h"
#include "boot.h"
#include "dbg.h"
#include "icu.h"
#include "user_config.h"
#include "gpio.h"
#include "icu.h"
#include "wdt.h"
#include "spi.h"
#include "adc.h"
#include "uart2.h"
#include "aon_rtc.h"
#include "rf.h"
#ifdef __AES_TEST__
#include "aes.h"
#endif
#if(USB_DRIVER)
#include "usb.h"
#include "driver_icu.h"
#endif
#include "Application_mode.h"
#include "rf_handler.h"
#include "my_drv_uart2.h"
#include "driver_timer.h"
#include "rf.h"
#include "timer_handler.h"
#include "bat_protocol.h"


extern void  xvr_reg_initial_24(void);
uint8_t uart_rx_en;


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

void test_rf_app(void)
{
    uart_printf("test_rf_app\r\n");
    uint8_t test_data[32];
    RF_txQueue_Send(test_data, 32);
}


void platform_reset(uint32_t error)
{

    uart_printf("reset error = %x\r\n", error);
    // Disable interrupts
    GLOBAL_INT_STOP();

    cpu_reset();

}
#ifdef __AES_TEST__
extern uint8_t aes_ok;
extern uint8_t encrypted_data[16];

uint8_t value[16]={0x00, 0x01 ,0x02 ,0x03 ,0x04 ,0x05, 0x06 ,0x07 ,0x08,
    0x09 ,0x0A ,0x0B, 0x0C ,0x0D, 0x0E ,0x0F};
uint8_t key[16]={0xED, 0x8D, 0x9C, 0x50, 0xD1, 0x55, 0xAA, 0x1E, 0x4D, 0x8F,
    0xF5, 0x81, 0x13, 0x32, 0x4F, 0x04, };
uint8_t encrypted_data[16]={0x37, 0x37, 0x79, 0x52, 0x26, 0x2E, 0x35, 0x08,
    0xAA, 0x12, 0x32, 0x2D, 0x99, 0xB3, 0xEC, 0x36 };

#endif
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
uint8_t user_flash_data1[16] = {0x12,0x34,0xaa,0x00,0x00,0x20,0x00,0x00,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,};
void Ep2_RxCbk(void*ptr,int sz){
//    int i;
    /*�iif(((char*)ptr)[0]==0x10){
        mouseTest=1;
        uart_printf("recieved a cmd\r\n");
    }
    AplUsb_StartTx(USB_ENDPID_Hid_DBG_IN, ptr, sz);
    
    
//    print("Ep3 Rx Data",ptr,sz);
    */
        char * pBuffer;
        pBuffer = (char*)ptr;
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

}

void MicIn_cbk(void*ptr,int sz){
    signed short int src[16];
    memset(src,0,sizeof(src));
    int i;
    for(i=0;i<sizeof(src)/sizeof(src[0]);i++){
        src[i]=i*0x100+i;
    }
    memcpy(ptr,src,sizeof(src));
    AplUsb_StartTx(USB_ENDPID_Audio_MIC,src,sizeof(src));
//     uart_send("RX",2);
}
void print(void*msg,void*dat,int len){
    char*ptr=(char*)dat;
    int i;
    uart_printf(msg);
    uart_printf("[%d]={\r\n\t",len);
    for(i=0;i<len;i++){
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
uint8_t test_sand_data[32];

int main(void)
{
    icu_init();
    //wdt_disable();
    intc_init();
    
    #if(UART_PRINTF_ENABLE)
    #if(!USB_DRIVER)
    uart_init(115200);
    #endif
    uart_init(115200);
    uart2_init(115200);//
    #endif
    uart2_printf("main start2~~~~~\r\n");
    uart_printf("main start~~~~~\r\n");

    //定时器初始化
    Timer_Handler_Init();

    gpio_init();
    flash_init();
  //  xvr_reg_initial_24();
  //  gpio_set_neg(0x04);

    xvr_reg_initial();
    #ifdef __USB_TEST__
    mcu_clk_switch(MCU_CLK_64M);
    #else
    mcu_clk_switch(MCU_CLK_16M);
    #endif   
    #if(AON_RTC_DRIVER)
    aon_rtc_init();
    #endif
    #if(SPI_DRIVER)
    spi_init(0,0,0);
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

    gpio_cb_register(app_gpio_sleep);
    GLOBAL_INT_START();
 
    uart_printf("main start~~~~~\r\n");
    #ifdef __USB_TEST__
    uint8_t ms_buf[16];
    // send media key ,no id,the another endpoint
    gpio_config(0x10,0,0);
     ms_buf[0]= 0x00;
     ms_buf[1]= 0x00;
     ms_buf[2] = 0;
     ms_buf[3] = 0;
     ms_buf[4] = 0;
     ms_buf[5] = 0;
     ms_buf[6] = 0;
     ms_buf[7] = 0;
     ms_buf[8] = 0;
     ms_buf[9] = 0;

    while(1)
    {
        if(gpio_get_input(0x10)==0)
        {
            if(ms_buf[0]== 0)
            {
                ms_buf[0]= 0x02;
                AplUsb_StartTx(USB_ENDPID_Hid_MKEY, ms_buf, 2);
                uart_printf("send R \r\n");
            }
        }
        else
        {
            if(ms_buf[0]== 0x02)
            {
            ms_buf[0]= 0x00;
            AplUsb_StartTx(USB_ENDPID_Hid_MKEY, ms_buf, 2);
            uart_printf("release \r\n");
            }
        }
        Delay_ms(100);
    }
    // send keyboard
    gpio_config(0x10,0,0);
    ms_buf[0]= 0x02;
    ms_buf[1]= 0x00;
    ms_buf[2] = 0;
    ms_buf[3] = 0;
    ms_buf[4] = 0;
    ms_buf[5] = 0;
    ms_buf[6] = 0;
    ms_buf[7] = 0;
    ms_buf[8] = 0;
    ms_buf[9] = 0;

    while(1)
    {
        if(gpio_get_input(0x10)==0)
        {
            if(ms_buf[1]== 0)
            {
                ms_buf[1]= 0x02;
                AplUsb_StartTx(USB_ENDPID_Hid_MSE, ms_buf, 9);
                uart_printf("send R \r\n");
            }
        }
        else
        {
            if(ms_buf[1]== 0x02)
            {
                ms_buf[1]= 0x00;
                AplUsb_StartTx(USB_ENDPID_Hid_MSE, ms_buf, 9);
                uart_printf("release \r\n");
            }
        }
        Delay_ms(100);
    }
      // send mouse
    gpio_config(0x10,0,0);
    ms_buf[0]= 0x01;
    ms_buf[1]= 0x00;
    ms_buf[2] = 0;
    ms_buf[3] = 0;
    ms_buf[4] = 0;
    ms_buf[5] = 0;
    ms_buf[6] = 0;
    ms_buf[7] = 0;
    ms_buf[8] = 0;
    ms_buf[9] = 0;

    while(1)
    {
        if(gpio_get_input(0x10)==0)
        {
            static int16_t deltaX,deltaY;
            //if(ms_buf[1]== 0)
            {
                static uint8_t dcount = 100;
                dcount++;
                if(dcount>=100)
                {
                    dcount = 0;
                    if ((deltaX == 0) && (deltaY == 4))
                    {
                        deltaX=4;
                        deltaY = 0;
                    }
                    else if ((deltaX == 4) && (deltaY == 0))
                    {
                        deltaX = 0;
                        deltaY = -4;
                    }
                    else if ((deltaX == 0) && (deltaY== -4))
                    {
                        deltaX = -4;
                        deltaY = 0;
                    }
                    else if((deltaX == -4) && (deltaY == 0))
                    {
                        deltaX = 0;
                        deltaY = 4;
                    }
                    else
                    {
                        deltaX = 4;
                        deltaY = 0;
                    }
                }
                ms_buf[1]= 0x02;
                ms_buf[2] = deltaX;
                ms_buf[3] = ((deltaY<<4)&0xf0)|((deltaX>>8)&0x0f);
                ms_buf[4] = deltaY>>4;
                AplUsb_StartTx(USB_ENDPID_Hid_MSE, ms_buf, 6);
                uart_printf("send R \r\n");
                Delay_ms(8);
            }
        }
        else
        {
            if(ms_buf[1]== 0x02)
            {
                ms_buf[1]= 0x00;
                AplUsb_StartTx(USB_ENDPID_Hid_MSE, ms_buf, 6);
                uart_printf("release \r\n");
            }
        }
        Delay_ms(100);
    }
    #endif
    #ifdef __AES_TEST__
    // aes test
    gpio_config(KEY_RIGHT,INPUT, PULL_HIGH);
    gpio_config(KEY_LEFT,INPUT, PULL_HIGH);
    gpio_config(KEY_MIDDLE,INPUT, PULL_HIGH);
    gpio_config(KEY_PAIR,INPUT, PULL_HIGH);
    while(1)
    {
        app_aes_test(key,value,encrypted_data);
        app_gpio_sleep();
        cpu_reduce_voltage_sleep();
        uart_printf("wake up\r\n");

        Delay_ms(1000);
    }
    while(1)
    {
        if(aes_ok)
        {
            aes_ok = 0;
            uart_printf("data=");
            for(i=0;i<16;i++)
                uart_printf("%x,",encrypted_data[i]);
            uart_printf("\r\n");
            while(1);
        }
    }
    #endif
    

    /* --------------------------------------简单RF收发测试---------------------------- */
     static uint8_t txcount=0;
     RF_Handler_Init();//初始化RF句柄及队列
    // while(1) //发送
    // {
    //     txcount++;
    //     test_sand_data[0] = txcount;
    //     //RF_txQueue_Send(test_sand_data, 32);//测试发送数据入队
    //     test_rf_app();
    //     RF_Service_Handler(&hrf);  //处理发送队列，卡死在这里
    //     Delay_ms(2);
    // }
    
    // HAL_RF_SetRxMode(&hrf);//设置为接收模式
    // while(1)//接收
    // {
    //     txcount++;
    //     uint8_t* rec_data;
    //     uint8_t  out_len;
    //     if(RF_rxQueue_Recv(&rec_data, &out_len)!=0){
    //         uart_printf("rec_data=");
    //         for(int i=0;i<out_len;i++)
    //             uart_printf("%x,",rec_data[i]);
    //         uart_printf("\r\n");
    //     }
    //     Delay_ms(1);
    // }

    /*----------------------------电池透传逻辑----------------------------*/
    RF_Handler_Init();//初始化RF句柄及队列
    uint8_t rec_data[32];
    //简易调度器，根据时间戳调用task
    while(1){
        //任务调用周期分别为 5ms 10ms 20ms 50ms
        static uint32_t last_timestamp[10] = {0};

        //主逻辑状态机
        if((Get_SysTick_ms() - last_timestamp[0]) >= 2){
            //5ms任务
            //串口收到电池数据解包
            Protocol_ParseByte(&uart2_rxQueue);
            
            last_timestamp[0] = Get_SysTick_ms();
        }
        if((Get_SysTick_ms() - last_timestamp[1]) >= 5){
            //5ms
            // RF_Service_Handler(&hrf);  //RF发送服务处理函数，周期200ms就不能正常发送？？？why???
            last_timestamp[1] = Get_SysTick_ms();
        }

        if((Get_SysTick_ms() - last_timestamp[2]) >= 20){
            //20ms任务
            last_timestamp[2] = Get_SysTick_ms();
        }

        if((Get_SysTick_ms() - last_timestamp[3]) >= 50){
            //50ms任务
            last_timestamp[3] = Get_SysTick_ms();
        }

        //100ms任务
        if((Get_SysTick_ms() - last_timestamp[4]) >= 100){
            last_timestamp[4] = Get_SysTick_ms();
        }

        //200ms任务
        if((Get_SysTick_ms() - last_timestamp[5]) >= 200){
            //RF_Service_Handler(&hrf);  //RF发送服务处理函数
            last_timestamp[5] = Get_SysTick_ms();
        }
        //500ms任务
        if((Get_SysTick_ms() - last_timestamp[6]) >= 500){
            RF_Service_Handler(&hrf);
            last_timestamp[6] = Get_SysTick_ms();
        }
        //1000ms任务
        if((Get_SysTick_ms() - last_timestamp[7]) >= 1000){
            last_timestamp[7] = Get_SysTick_ms();
            // uart_printf("--------------1000ms-task---------------------- \r\n");
            // if(RF_rxQueue_Recv(rec_data,32)){
            //     uart_printf("rf_rec_data=");
            //     for(int i=0;i<32;i++)
            //         uart_printf("%x,",rec_data[i]);
            //     uart_printf("\r\n");
            // }

            // uart_printf("--------------500ms-task---------------------- \r\n");
            // uint16_t counts = queue_get_counts(&uart2_rxQueue);
            // uart_printf("uart2_rxQueue counts=%d\r\n", counts);
            // uart_printf("queue_datas=: ");
            // for(int i=0;i<counts;i++)
            // {
            //     uint8_t data;
            //     queue_peek_at(&uart2_rxQueue, i, &data);
            //     uart_printf("%02X ", data);
            // }
            // uart_printf("\r\n");
        }
    }


/*-----------------------------------串口测试---------------------------*/
    while(1)
    {
        //uart_printf("in main loop\r\n");
        //串口queue测试
        uart_printf("tick=%d\r\n", Get_SysTick_ms());

        if(!queue_is_empty((&uart2_rxQueue)))
        {
            uint8_t byte;
            queue_pop(&uart2_rxQueue, &byte);
            uart_printf("%d\r\n", byte);
        }

    }
    
   
    while(1)
    {
        stack_integrity_check();
    }
}






