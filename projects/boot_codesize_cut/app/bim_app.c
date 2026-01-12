#include "bim_updataImage.h"
#include "bim_app.h"
#include "bim_uart2.h"
#include "bim_uart0.h"
#include "bim_flash.h"
#include "bim_icu.h"
#include "bim_wdt.h"
#include "bim_adc.h"


#if defined(__CC_ARM)
const  unsigned int BK36[] __attribute__((at(0x100)))= 
{
    0x36334B42,0x00103333,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
};
#else
volatile  unsigned int BK36[] __attribute__((section(".section_bk")))=
{
    0x36334B42,0x00103333,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
};
#endif

void updata_memset32(void * dest, uint32 value, uint8 size)
{
    uint32 *_u8_dest = (uint32 *)dest;
    uint32 *_u8_end  = (uint32 *)dest+size*4;

    while (_u8_dest < _u8_end)
    {
        *_u8_dest++ = value;
    }
}

typedef void (*FUNCPTR)(void);
extern void Delay_ms(int num);
extern void arm9_enable_alignfault(void);
extern uint32_t flash_mid;
void bim_main(void)
{
    Delay_ms(50);
    #if defined(__CC_ARM)
    arm9_enable_alignfault();
    #endif
    icu_init();
    wdt_disable();
    updata_memset32((uint8 *)0x00400000, 1, 1);
    uart0_init(115200);
    bim_printf("boot_start1\r\n");
    
    GLOBAL_INT_START();
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
        bim_printf("low volt start!!!\n");
        Delay_ms(100);
    }
    #endif
    
    flash_advance_init();
    
    bim_printf("flash_mid = 0x%x\r\n", flash_mid);
    if(1 == bim_select_sec())
    {
        updata_memset32((uint8 *)0x00400000, 0, 1);
        GLOBAL_INT_STOP();
        (*(FUNCPTR)SEC_IMAGE_RUN_STACK_CADDR)();
    }
    else
    {
        updata_memset32((uint8 *)0x00400000, 0, 1);
        GLOBAL_INT_STOP();
        while(1)
        {
            bim_printf("error_start\r\n");
        }
    }
}





