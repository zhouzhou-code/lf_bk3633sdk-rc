#include <stdint.h>        // standard integer definition
#include <string.h>        // string manipulation
#include "bim_flash.h"         // flash definition
#include "bim_uart2.h"
#include "bim_wdt.h"
#include "bim_adc.h"
#include "bim_updataImage.h"


/// Flash environment structure variable
struct flash_env_tag flash_env;
extern uint8_t system_mode;
static uint8_t flash_enable_write_flag1;
static uint8_t flash_enable_write_flag2;
static uint8_t flash_enable_write_flag3;
static uint8_t flash_enable_write_flag4;
static uint8_t flash_enable_erase_flag1;
static uint8_t flash_enable_erase_flag2;
uint32_t flash_mid = 0;

void set_flash_info()
{
    uint32_t flash_type = flash_mid & 0xff;
    if(flash_type==0x14)
    {
        flash_env.ota_all_image_start_faddr_abs = SEC_ALL_IMAGE_ALLOC_START_8M_FADDR;
        flash_env.ota_all_image_end_faddr_abs = SEC_ALL_IMAGE_ALLOC_END_8M_FADDR;
        flash_env.ota_all_backup_start_faddr_abs = SEC_ALL_BACKUP_OAD_HEADER_8M_FADDR;
        flash_env.ota_all_backup_end_faddr_abs =SEC_ALL_BACKUP_OAD_END_8M_FADDR;
        flash_env.ota_all_backup_oad_header_faddr_abs = SEC_ALL_BACKUP_OAD_HEADER_8M_FADDR;
        flash_env.ota_all_backup_oad_image_faddr_abs = SEC_ALL_BACKUP_OAD_IMAGE_8M_FADDR;

        flash_env.ota_part_image_start_faddr_abs = SEC_PART_IMAGE_ALLOC_START_8M_FADDR;
        flash_env.ota_part_image_end_faddr_abs = SEC_PART_IMAGE_ALLOC_END_8M_FADDR;
        flash_env.ota_part_backup_start_faddr_abs = SEC_PART_BACKUP_OAD_HEADER_8M_FADDR;
        flash_env.ota_part_backup_end_faddr_abs = SEC_PART_BACKUP_OAD_END_8M_FADDR;
        flash_env.ota_part_backup_oad_header_faddr_abs = SEC_PART_BACKUP_OAD_HEADER_8M_FADDR;
        flash_env.ota_part_backup_oad_image_faddr_abs = SEC_PART_BACKUP_OAD_IMAGE_8M_FADDR;
    }
    else
    {
        flash_env.ota_all_image_start_faddr_abs = SEC_ALL_IMAGE_ALLOC_START_4M_FADDR;
        flash_env.ota_all_image_end_faddr_abs = SEC_ALL_IMAGE_ALLOC_END_4M_FADDR;
        flash_env.ota_all_backup_start_faddr_abs = SEC_ALL_BACKUP_OAD_HEADER_4M_FADDR;
        flash_env.ota_all_backup_end_faddr_abs =SEC_ALL_BACKUP_OAD_END_4M_FADDR;
        flash_env.ota_all_backup_oad_header_faddr_abs = SEC_ALL_BACKUP_OAD_HEADER_4M_FADDR;
        flash_env.ota_all_backup_oad_image_faddr_abs = SEC_ALL_BACKUP_OAD_IMAGE_4M_FADDR;

        flash_env.ota_part_image_start_faddr_abs = SEC_PART_IMAGE_ALLOC_START_4M_FADDR;
        flash_env.ota_part_image_end_faddr_abs = SEC_PART_IMAGE_ALLOC_END_4M_FADDR;
        flash_env.ota_part_backup_start_faddr_abs = SEC_PART_BACKUP_OAD_HEADER_4M_FADDR;
        flash_env.ota_part_backup_end_faddr_abs = SEC_PART_BACKUP_OAD_END_4M_FADDR;
        flash_env.ota_part_backup_oad_header_faddr_abs = SEC_PART_BACKUP_OAD_HEADER_4M_FADDR;
        flash_env.ota_part_backup_oad_image_faddr_abs = SEC_PART_BACKUP_OAD_IMAGE_4M_FADDR;
    }
}
void set_flash_clk(unsigned char clk_conf)
{
    //note :>16M don't use la for flash debug
    unsigned int temp0;
    temp0 = REG_FLASH_CONF;
    REG_FLASH_CONF = (  (clk_conf << BIT_FLASH_CLK_CONF)
                        | (temp0    &  SET_MODE_SEL)
                        | (temp0    &  SET_FWREN_FLASH_CPU)
                        | (temp0    &  SET_WRSR_DATA)
                        | (temp0    &  SET_CRC_EN));
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
}
uint32_t get_flash_ID(void)
{
    unsigned int temp0;
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
    REG_FLASH_OPERATE_SW = (       FLASH_ADDR_FIX
                                   | (FLASH_OPCODE_RDID << BIT_OP_TYPE_SW)
                                   | (0x1               << BIT_OP_SW));
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
    for (temp0=0; temp0<8; temp0++)
    {
        REG_FLASH_DATA_SW_FLASH = 0xffffffff;
    }
    return REG_FLASH_RDID_DATA_FLASH ;
}

uint32_t flash_read_sr(void)
{
    uint32_t temp = 0;

    while(REG_FLASH_OPERATE_SW & 0x80000000);
    REG_FLASH_OPERATE_SW = ( FLASH_ADDR_FIX
                            | (FLASH_OPCODE_RDSR << BIT_OP_TYPE_SW)
                            | (0x1 << BIT_OP_SW));

    while(REG_FLASH_OPERATE_SW & 0x80000000);

    temp = (REG_FLASH_SR_DATA_CRC_CNT&0xff);

    REG_FLASH_OPERATE_SW = (FLASH_ADDR_FIX
                            | (FLASH_OPCODE_RDSR2 << BIT_OP_TYPE_SW)
                            | (0x1 << BIT_OP_SW));

    while(REG_FLASH_OPERATE_SW & 0x80000000);
    temp |= (REG_FLASH_SR_DATA_CRC_CNT&0xff) << 8;
    return temp ;
}

void flash_write_sr_temp( uint8_t bytes,  uint16_t val )
{
    if(flash_mid != get_flash_ID())
    {
        return;
    }
    switch(flash_mid)
    {
    case MX_FLASH_4M:
    case MX_FLASH_1:               //MG xx
        REG_FLASH_CONF &= 0xffdf0fff;
        break;
    case GD_FLASH_1:              //QD xx ,
    case BY25Q80:
    case PN25f04:
        REG_FLASH_CONF &= 0xfefe0fff;
        break;
     case P25Q40U:
     case TH25D40HB:
     case GT25Q40:    
        REG_FLASH_CONF &= 0xfef00fff;
    break;
    case GD_25WD80E:
        REG_FLASH_CONF &= 0xfffe0fff;
    break;
    case XTX_FLASH_1:              //XTX xx
    case GD_MD25D40:
    case GD_GD25WD40:
    default:
        REG_FLASH_CONF &= 0xffff0fff;
        break;
    }
    if(bytes==0||bytes>2)
    {
        return;
    }
    REG_FLASH_CONF |= (val << BIT_WRSR_DATA)|SET_FWREN_FLASH_CPU;
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
    if(flash_mid != get_flash_ID())
    {
        return;
    }
    if( bytes == 1 )
    {
        REG_FLASH_OPERATE_SW = (FLASH_ADDR_FIX|(FLASH_OPCODE_WRSR << BIT_OP_TYPE_SW)
                                | (0x1<< BIT_OP_SW)
                                | (0x1<< BIT_WP_VALUE));
    }
    else if(bytes == 2 )
    {
        REG_FLASH_OPERATE_SW = (FLASH_ADDR_FIX|(FLASH_OPCODE_WRSR2 << BIT_OP_TYPE_SW)
                                | (0x1<< BIT_OP_SW)
                                | (0x1<< BIT_WP_VALUE));
    }
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
    REG_FLASH_OPERATE_SW = FLASH_ADDR_FIX;
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
}

void flash_write_sr( uint8_t bytes,  uint16_t val )
{
    static uint8_t write_sr_cnt = 0;
    uint16_t sr_data=0;
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
        bim_printf("low volt \n");
    }
    #endif

    sr_data=flash_read_sr();
    bim_printf("sr_data = 0x%x,VAL=%x\r\n", sr_data,val);
    if(flash_mid==GD_GD25WD40 || flash_mid==GD_25WD80E)
    {
        if((sr_data&0xbf)==val)
            return;
    }
    if(sr_data == val)
    {
        return; 
    }

    flash_write_sr_temp(bytes, val);
    sr_data=flash_read_sr();
    if(flash_mid==GD_GD25WD40 || flash_mid==GD_25WD80E)
    {
        sr_data &= 0xbf;
    }

    while (sr_data != val)
    {
        flash_write_sr_temp(bytes, val);

        write_sr_cnt++;
        if(write_sr_cnt > 10)
        {
            //uart_printf("boot write sr error! WDT_RESET!!!\r\n");
            //wdt_enable(0x10);
            bim_printf("write sr error! cpu_reset!!!\r\n");
            bim_printf("sr = 0x%x\r\n", flash_read_sr());
            extern void cpu_reset(void);
            cpu_reset();
            while(1);
        }
        sr_data=flash_read_sr();
        if(flash_mid==GD_GD25WD40 || flash_mid==GD_25WD80E)
        {
            sr_data &= 0xbf;
        }
    }
}

void flash_wp_8k( void )
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
    bim_printf("low volt 2!!!\n");
    }
    #endif
    
    flash_sr=flash_read_sr( );

    switch(flash_mid)
    {
    case MX_FLASH_4M:
    case MX_FLASH_1:               //MG xx
        if(flash_sr!=0x0884)
            flash_write_sr( 2, 0x0884 );
        break;
    case GD_FLASH_1:              //QD xx ,
    case BY25Q80:
    case PN25f04:
        if(flash_sr!=0x00e8)
            flash_write_sr( 2, 0x00e8 );
        break;
    // add new flash
    case P25Q40U:
    case TH25D40HB:
    case GT25Q40:    
        if(flash_sr!=0x0000)
            flash_write_sr( 2, 0x0000 );
        break;
    case XTX_FLASH_1:              //XTX xx
    case GD_MD25D40:
    case GD_GD25WD40:
    default:
        if(flash_sr!=0x00)
            flash_write_sr( 1, 0x00 );
        break;
    }
}

void flash_wp_128k( void )
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
        bim_printf("low volt 3!!!\n");
    }
    #endif
    flash_sr=flash_read_sr( );


    switch(flash_mid)
    {
    case MX_FLASH_4M:
    case MX_FLASH_1:               //MG xx
        if(flash_sr!=0x0888)
            flash_write_sr( 2, 0x0888 );
        break;
    case XTX_FLASH_1:              //XTX xx
        if(flash_sr!=0xA8)
            flash_write_sr( 1, 0xA8 );
        break;
    case GD_FLASH_1:              //QD xx ,
    case BY25Q80:
    case PN25f04:
        if(flash_sr!=0x00A8)
            flash_write_sr( 2, 0x00A8 );
        break;
    // add new flash
    case P25Q40U:
    case TH25D40HB:
    case GT25Q40:    
        if(flash_sr!=0x0028)
            flash_write_sr( 2, 0x0028 );
        break;
    case GD_MD25D40:
    case GD_GD25WD40:
    default:
        if(flash_sr!=0x00)
            flash_write_sr( 1, 0x00 );
        break;
    }
}
void flash_wp_256k( void)
{

    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
    bim_printf("low volt 4!!!\n");
    }
    #endif
    
    flash_sr=flash_read_sr( );
    switch(flash_mid)
    {
    case MX_FLASH_4M:
    case MX_FLASH_1:               //MG xx
        if(flash_sr!=0x088C)
            flash_write_sr( 2, 0x088C );
        break;
    case XTX_FLASH_1:              //XTX xx
        if(flash_sr!=0xAC)
            flash_write_sr( 1, 0xAC );
        break;
    case GD_FLASH_1:              //QD xx ,
    case BY25Q80:
    case PN25f04:
        if(flash_sr!=0x00ac)
            flash_write_sr( 2, 0x00ac );
        break;
    case P25Q40U:
    case TH25D40HB:
    case GT25Q40:    
        if(flash_sr!=0x002c)
            flash_write_sr( 2, 0x002c );
        break;
    case GD_25WD80E:
        if(flash_sr!=0x98)
            flash_write_sr(1, 0x98);  //portect 0 ~ 768KB area
        break;
    case GD_MD25D40:
    case GD_GD25WD40:
    default:
        if(flash_sr!=0x98)
            flash_write_sr( 1, 0x98 );
        break;
    }
}

void flash_wp_all( void )
{
    //  add read sr for proctol sr reg  ___yubei__2021/7/8
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
        bim_printf("low volt 5!!!\n");
    }
    #endif
    flash_sr=flash_read_sr( );

    switch(flash_mid)
    {
    case MX_FLASH_4M:
    case MX_FLASH_1:               //MG xx
        if(flash_sr!=0x00bc)
            flash_write_sr( 2, 0x00bc );
        break;
    case XTX_FLASH_1:              //XTX xx
        if(flash_sr!=0xBC)
            flash_write_sr( 1, 0xBC );
        break;
    case GD_FLASH_1:              //QD xx ,
    case BY25Q80:
    case PN25f04:
        if(flash_sr!=0x0094)
            flash_write_sr( 2, 0x0094 );
        break;

    // add new flash
    case P25Q40U:
    case TH25D40HB:
    case GT25Q40:    
        if(flash_sr!=0x0010)
            flash_write_sr( 2, 0x0010 );
        break;
    case GD_MD25D40:
    case GD_GD25WD40:
    case GD_25WD80E:
    default:
        if(flash_sr!=0x9c)
            flash_write_sr( 1, 0x9c );
        break;
    }
}

void flash_wp_none( void)
{
    uint32_t flash_sr;
    #ifdef CHECK_LOW_VOLT_ENABLE
    while(check_low_volt_sleep())
    {
        bim_printf("low volt\n");
    }
    #endif
    flash_sr=flash_read_sr( );
    switch(flash_mid)
    {
        case GD_25WD80E:
        if(flash_sr!=0x80)
        flash_write_sr(1, 0x80);  //portect none area
        break;

        default:
        break;    
    }
}

void flash_advance_init(void)
{
    //uint32_t flash_sr;
    flash_mid = get_flash_ID();
    set_flash_info();
    //flash_sr=flash_read_sr( );
    //flash_wp_256k();
}

void flash_erase_sector(uint32_t address)
{
    GLOBAL_INT_DISABLE();
    if (address<0x2000)
    {
        return;
    }
    if(flash_enable_erase_flag1==FLASH_ERASE_ENABLE1&&flash_enable_erase_flag2==FLASH_ERASE_ENABLE2)
    {
        
        while(REG_FLASH_OPERATE_SW & 0x80000000)
        {
            ;
        }
        REG_FLASH_OPERATE_SW = (  (address << BIT_ADDRESS_SW)
                                  | (FLASH_OPCODE_SE<< BIT_OP_TYPE_SW)
                                  | (0x1             << BIT_OP_SW));
        while(REG_FLASH_OPERATE_SW & 0x80000000)
        {
            ;
        }
    }
    GLOBAL_INT_RESTORE();
}
void flash_erase_one_block(uint32_t address)
{
    GLOBAL_INT_DISABLE();
    if (address<0x2000)
    {
        return;
    }
    //if(flash_enable_erase_flag1==FLASH_ERASE_ENABLE1&&flash_enable_erase_flag2==FLASH_ERASE_ENABLE2)
    {
        while(REG_FLASH_OPERATE_SW & 0x80000000)
        {
            ;
        }
        REG_FLASH_OPERATE_SW = (  (address << BIT_ADDRESS_SW)
                                  | (FLASH_OPCODE_BE2 << BIT_OP_TYPE_SW)
                                  | (0x1             << BIT_OP_SW));
        while(REG_FLASH_OPERATE_SW & 0x80000000)
        {
            ;
        }
    }
    GLOBAL_INT_RESTORE();
}
void flash_read_data (uint8_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t i;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8];
    uint8_t *pb = (uint8_t *)&buf[0];

    if (len == 0)
        return;
    GLOBAL_INT_DISABLE();
    while(REG_FLASH_OPERATE_SW & 0x80000000);

    while (len)
    {
        REG_FLASH_OPERATE_SW = (  (addr << BIT_ADDRESS_SW)
                                | (FLASH_OPCODE_READ << BIT_OP_TYPE_SW)
                                | (0x1 << BIT_OP_SW));
        while(REG_FLASH_OPERATE_SW & 0x80000000);
        addr+=32;

        for (i = 0; i < 8; i++)
            buf[i] = REG_FLASH_DATA_FLASH_SW;

        for (i = (address & 0x1F); i < 32; i++)
        {
            *buffer++ = pb[i];
            address++;
            len--;
            if (len == 0)
                break;
        }
    }
    REG_FLASH_OPERATE_SW=FLASH_ADDR_FIX ;
    for (i=0; i<8; i++)
        REG_FLASH_DATA_SW_FLASH = 0xffffffff;
    GLOBAL_INT_RESTORE();
}

void flash_write_data (uint8_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t  i;
    uint32_t addr = address&(~0x1F);
    uint32_t buf[8] = {~0x00UL};
    uint8_t *pb = (uint8_t *)&buf[0];
    if (len == 0)
        return;
    if (address<0x2000)
    {
        return;
    }
    GLOBAL_INT_DISABLE();
    while(REG_FLASH_OPERATE_SW & 0x80000000)
    {
        ;
    }
    flash_enable_write_flag3=FLASH_WRITE_ENABLE3;

    while(len)
    {
        if((address & 0x1F) || (len < 32))
            flash_read_data(pb, addr, 32);

        for(i = (address & 0x1F); i < 32; i++)
        {
            if(len)
            {
                pb[i] = *buffer++;
                address++;
                len--;
            }
        }

        flash_enable_write_flag4=FLASH_WRITE_ENABLE4;
        for (i=0; i<8; i++)
            REG_FLASH_DATA_SW_FLASH = buf[i];


        if(flash_enable_write_flag1==FLASH_WRITE_ENABLE1 && flash_enable_write_flag2==FLASH_WRITE_ENABLE2 )
        {
            while(REG_FLASH_OPERATE_SW & 0x80000000);

            if(flash_enable_write_flag3==FLASH_WRITE_ENABLE3 && flash_enable_write_flag4==FLASH_WRITE_ENABLE4)
            {
                if(addr<0x2000)
                {
                    return;
                }
                REG_FLASH_OPERATE_SW = (  (addr << BIT_ADDRESS_SW)
                                          | (FLASH_OPCODE_PP << BIT_OP_TYPE_SW)
                                          | (0x1 << BIT_OP_SW));
            }
            while(REG_FLASH_OPERATE_SW & 0x80000000);
        }
        addr+=32;
    }
    
    REG_FLASH_OPERATE_SW=FLASH_ADDR_FIX ;
    flash_enable_write_flag3=0;
    flash_enable_write_flag4=0;
    for (i=0; i<8; i++)
    {
        REG_FLASH_DATA_SW_FLASH = 0xffffffff;
    }
    GLOBAL_INT_RESTORE();
}


uint8_t flash_read( uint8_t *buffer,uint32_t address, uint32_t len)
{
    uint32_t pre_address;
    uint32_t post_address;
    uint32_t pre_len;
    uint32_t post_len;
    uint32_t page0;
    uint32_t page1;
    page0 = address &(~FLASH_PAGE_MASK);
    page1 = (address + len) &(~FLASH_PAGE_MASK);
    if(page0 != page1)
    {
        pre_address = address;
        pre_len = page1 - address;
        flash_read_data(buffer, pre_address, pre_len);
        post_address = page1;
        post_len = address + len - page1;
        flash_read_data((buffer + pre_len), post_address, post_len);
    }
    else
    {
        flash_read_data(buffer, address, len);
    }
    
    return 0;
}
uint8_t flash_write(uint8_t *buffer,uint32_t address, uint32_t len)
{
    uint32_t pre_address;
    uint32_t post_address;
    uint32_t pre_len;
    uint32_t post_len;
    uint32_t page0;
    uint32_t page1;

    if(flash_mid != get_flash_ID())
    {
        //bim_printf("flash = 0x%x\r\n", get_flash_ID());
        return 1;
    }
    flash_enable_write_flag1=FLASH_WRITE_ENABLE1;
    page0 = address &(~FLASH_PAGE_MASK);
    page1 = (address + len) &(~FLASH_PAGE_MASK);

    if(page0 != page1)
    {
        pre_address = address;
        pre_len = page1 - address;
        flash_enable_write_flag2=FLASH_WRITE_ENABLE2;
        flash_write_data(buffer, pre_address, pre_len);

        post_address = page1;
        post_len = address + len - page1;
        flash_write_data((buffer + pre_len), post_address, post_len);

    }
    else
    {
        flash_enable_write_flag2=FLASH_WRITE_ENABLE2;
        flash_write_data(buffer, address, len);

    }
    flash_enable_write_flag1=0;
    flash_enable_write_flag2=0;
    return 0;
}

////2¨¢3yo¡¥¨ºy¡ê?¦Ì??¡¤o¨ª3¡è?¨¨??¨°a4K¦Ì???¨ºy¡À?
uint8_t flash_erase( uint32_t address, uint32_t len)
{
    /* assume: the worst span is four sectors*/
    int erase_addr;
    int erase_len;
    flash_enable_erase_flag1=FLASH_ERASE_ENABLE1;
    if(flash_mid != get_flash_ID())
    {
        return 1;
    }
    {
        erase_addr = address & (~FLASH_ERASE_SECTOR_SIZE_MASK);
        erase_len = len;
    }
    do
    {
        int i;
        int erase_whole_sector_cnt;
        //erase_whole_sector_cnt = erase_len >> FLASH_ERASE_SECTOR_SIZE_RSL_BIT_CNT;
        erase_whole_sector_cnt = erase_len/FLASH_ERASE_SECTOR_SIZE + (erase_len%FLASH_ERASE_SECTOR_SIZE>0? 1:0);
        flash_enable_erase_flag2=FLASH_ERASE_ENABLE2;
        for(i = 0; i < erase_whole_sector_cnt; i ++)
        {
            flash_erase_sector(erase_addr);
            erase_addr += FLASH_ERASE_SECTOR_SIZE;
            //erase_len -= FLASH_ERASE_SECTOR_SIZE;
        }
    }
    while(0);
    flash_enable_erase_flag1=0;
    flash_enable_erase_flag2=0;
    return 0;
}

