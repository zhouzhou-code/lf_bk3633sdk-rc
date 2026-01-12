#include "bim_updataImage.h"
#include "bim_icu.h"
#include "bim_uart2.h"
#include "bim_flash.h"
#include "bim_wdt.h"

#include <string.h>

extern uint32_t flash_mid; 

img_hdr_t hdr_img;
img_hdr_t hdr_back;
img_hdr_t hdr_back_part;
int make_crc32_table(void);
uint32_t make_crc32(uint32_t crc,unsigned char *string,uint32_t size);
uint32_t bim_image_header_check(void)
{
    //flash_read(0,SEC_IMAGE_OAD_HEADER_APP_FADDR,sizeof(img_hdr_t),(uint8_t *)&hdr_img,NULL);
    //if(hdr_img.uid != OAD_APP_PART_UID)
    //    return SSTATUS_SECT_UNKOWN;
    flash_read((uint8_t *)&hdr_img,SEC_IMAGE_OAD_HEADER_STACK_FADDR,sizeof(img_hdr_t));
    if((hdr_img.uid != OAD_APP_STACK_UID) && (hdr_img.uid != OAD_APP_PART_UID))
        return SSTATUS_SECT_UNKOWN;

    return 0;
}
uint32_t bim_get_psec_image_header(void)
{
    uint32_t sec_image_oad_header_fddr;
    if(hdr_back.uid == OAD_APP_PART_UID)
    {
        sec_image_oad_header_fddr = SEC_IMAGE_OAD_HEADER_APP_FADDR;
    }
    else if(hdr_back.uid == OAD_APP_STACK_UID)
    {
        sec_image_oad_header_fddr = SEC_IMAGE_OAD_HEADER_STACK_FADDR;
    }
    flash_read((uint8_t *)&hdr_img,sec_image_oad_header_fddr,sizeof(img_hdr_t));
#if 0
    bim_printf("hdr_img.crc = %x\n",hdr_img.crc);
    bim_printf("hdr_img.crc_status = %x\n",hdr_img.crc_status);
    bim_printf("hdr_img.len =%x\n ",hdr_img.len);
    bim_printf("hdr_img.rom_ver = %x\n",hdr_img.rom_ver);
    bim_printf("hdr_img.sec_status = %x\n",hdr_img.sec_status);
    bim_printf("hdr_img.ver = %x\n",hdr_img.ver);
    bim_printf("hdr_img.uid = %x\n",hdr_img.uid);
#endif
    return 0;
}

uint32_t bim_get_psec_backup_header(void)
{
    flash_read((uint8_t *)&hdr_back_part, flash_env.ota_part_backup_oad_header_faddr_abs,sizeof(img_hdr_t));

    bim_printf("hdr_back_part.uid = %x\n",hdr_back_part.uid);
    if(hdr_back_part.uid!=OAD_APP_PART_UID)
    {
        flash_read((uint8_t *)&hdr_back, flash_env.ota_all_backup_oad_header_faddr_abs,sizeof(img_hdr_t));
    }
    else
    {
        hdr_back.uid=hdr_back_part.uid;
        hdr_back.crc=hdr_back_part.crc;
        hdr_back.ver=hdr_back_part.ver;
        hdr_back.len=hdr_back_part.len;
        hdr_back.crc_status=hdr_back_part.crc_status;
        hdr_back.sec_status=hdr_back_part.sec_status;
        hdr_back.rom_ver=hdr_back_part.rom_ver;
    }
    #if 0
    bim_printf("hdr_back.crc = %x\n",hdr_back.crc);
    bim_printf("hdr_back.crc_status = %x\n",hdr_back.crc_status);
    bim_printf("hdr_back.len =%x\n ",hdr_back.len);
    bim_printf("hdr_back.rom_ver = %x\n",hdr_back.rom_ver);
    bim_printf("hdr_back.sec_status = %x\n",hdr_back.sec_status);
    bim_printf("hdr_back.ver = %x\n",hdr_back.ver);
    bim_printf("hdr_back.uid = %x\n",hdr_back.uid);
    #endif
    return 0;
}


uint32_t calc_image_sec_crc(void)
{
    uint8_t data[BLOCK_SIZE];
    //uint8_t tmp_data[BLOCK_SIZE];
    uint32_t block_total;
    uint32_t read_addr;
    uint32_t calcuCrc = 0xffffffff;
    make_crc32_table();
    block_total =  hdr_img.len / 4 - 1;// not clac HDR
    
    uint8_t less_block= hdr_img.len % 4;
    
    if(hdr_img.uid == OAD_APP_PART_UID)
    {
        read_addr = SEC_IMAGE_RUN_APP_FADDR;
    }
    else if(hdr_img.uid == OAD_APP_STACK_UID)
    {
        read_addr = SEC_IMAGE_RUN_STACK_FADDR;
    }
    //bim_printf("read start addr = %x\n",read_addr);
    for(uint32_t i = 0; i < block_total; i++)
    {
        flash_read(data,read_addr,BLOCK_SIZE);

        calcuCrc = make_crc32(calcuCrc,data,BLOCK_SIZE);

        read_addr+= BLOCK_SIZE;
    }
    if(less_block != 0)
    {
        flash_read(data,read_addr,less_block*4);
        calcuCrc = make_crc32(calcuCrc,data,less_block*4);
    }

    //bim_printf("read end addr = %x\n",read_addr);
    return calcuCrc;
}

uint32_t calc_backup_sec_crc(uint32_t addr)
{

    uint8_t data[BLOCK_SIZE];
  
    uint16_t block_total;
    uint32_t read_addr;
    uint32_t calcuCrc = 0xffffffff;
    make_crc32_table();
    block_total = hdr_back.len / 4 - 1;
    read_addr = addr;

    for(uint32_t i = 0; i < block_total; i++)
    {
        flash_read(data,read_addr,BLOCK_SIZE);
        
        calcuCrc = make_crc32(calcuCrc,data,BLOCK_SIZE);
        read_addr+= BLOCK_SIZE;

    }

    return calcuCrc;
}
//const uint32_t ROM_VER = 0x0005;
uint8_t bim_check_image_sec_status(void)
{
    bim_get_psec_image_header();
    if(hdr_img.uid == OAD_APP_PART_UID)
    {
        if(hdr_img.len != 0xffff )
        {
            if(hdr_img.crc == calc_image_sec_crc()) 
            {
                //bim_printf("image crc OK\r\n");
                return SSTATUS_SECT_NORMAL;
            }
        }
        return SSTATUS_SECT_ABNORMAL;
    }
    else if(hdr_img.uid == OAD_APP_STACK_UID)
    {
        if(hdr_img.len != 0xffff )
        {
            if(hdr_img.crc == calc_image_sec_crc()) 
            {
                //bim_printf("image crc OK\r\n");
                return SSTATUS_SECT_NORMAL;
            }
        }
        return SSTATUS_SECT_ABNORMAL;
    } 
    else
    {
        //bim_printf("bim_check_image_sec_status: ERROR, UNKNOWN UID\r\n");
        return SSTATUS_SECT_UNKOWN;
    }
}

uint8_t bim_check_backup_sec_status(void)//NOT WRITE INFO TO FLASH
{
    bim_get_psec_backup_header();
    if((hdr_back.rom_ver == 0xffff) || (hdr_back.ver == 0xffff)  )
    {
        return SSTATUS_SECT_ERASED;
    }

    if(hdr_back.uid == OAD_APP_PART_UID)
    {
        if(hdr_back.len != 0xffff )
        {
            if(hdr_back.crc == calc_backup_sec_crc(flash_env.ota_part_backup_oad_image_faddr_abs)) // crc ok
            {
                bim_printf("backup crc ok\r\n");
                return SSTATUS_SECT_NORMAL;
            }
        }
        return SSTATUS_SECT_ABNORMAL;
    }
    else if(hdr_back.uid == OAD_APP_STACK_UID)
    {
        if( hdr_back.len != 0xffff )
        {
            if(hdr_back.crc == calc_backup_sec_crc(flash_env.ota_all_backup_oad_image_faddr_abs)) // crc ok
            {
                bim_printf("backup crc ok\r\n");
                return SSTATUS_SECT_NORMAL;
            }
        }
        return SSTATUS_SECT_ABNORMAL;
    }  
    else
    {
        bim_printf("UNKNOWN\r\n");
        return SSTATUS_SECT_UNKOWN;
    }
}

void bim_erase(uint32_t start_addr,uint32_t end_addr)
{
    uint32_t block_addr;
    uint32_t sector_addr;
    sector_addr=(end_addr&0xfffff000);
    block_addr=(end_addr&0xffff0000);
    //bim_printf("sector_addr=%x\n",sector_addr);
    //bim_printf("block_addr=%x\n",block_addr);
    if(sector_addr>block_addr)
    {
        flash_erase(block_addr,sector_addr-block_addr);
    }
    while((block_addr-FLASH_ONE_BLOCK_SIZE)>start_addr )
    {
        block_addr -= FLASH_ONE_BLOCK_SIZE;
        //bim_printf("block_addr=%x\n",block_addr);
        flash_erase_one_block(block_addr);
    }
    flash_erase(start_addr,block_addr-start_addr);

}
void bim_erase_image_sec(void)
{
    //bim_printf("erase_image start \n");

    if(hdr_back.uid == OAD_APP_PART_UID)  //128k
    {
        bim_erase(flash_env.ota_part_image_start_faddr_abs,flash_env.ota_part_image_end_faddr_abs);
    }
    else if(hdr_back.uid == OAD_APP_STACK_UID) //248k
    {
        bim_erase(flash_env.ota_all_image_start_faddr_abs,flash_env.ota_all_image_end_faddr_abs);
    }
    else
    {
        bim_printf("bim_erase_image_sec: ERROR, UNKNOWN UID\r\n");
    }
    //bim_printf("erase_image end \n");
}
void bim_erase_backup_sec(void)
{
    if(hdr_back.uid==OAD_APP_STACK_UID)
    {
        bim_erase(flash_env.ota_all_backup_start_faddr_abs,flash_env.ota_all_backup_end_faddr_abs);

        //bim_printf("erase all backup addr \r\n");
    }
    else if(hdr_back.uid==OAD_APP_PART_UID)
    {
        bim_erase(flash_env.ota_part_backup_start_faddr_abs,flash_env.ota_part_backup_end_faddr_abs);
        //bim_printf("erase part backup addr\r\n");
    }
}
void bim_updata_backup_to_image_sec(void)
{
    uint8_t data[READ_BLOCK_SIZE];
    uint32_t backup_size = hdr_back.len * 4;
    uint32_t read_end_addr ;
    uint32_t read_addr;
    uint32_t write_addr;
    bim_printf("backup_size =%x\n",backup_size);
    if(hdr_back.uid == OAD_APP_PART_UID) // only app part
    {
        write_addr = SEC_IMAGE_OAD_HEADER_APP_FADDR;
        read_end_addr = flash_env.ota_part_backup_oad_header_faddr_abs + backup_size;
        read_addr = flash_env.ota_part_backup_oad_header_faddr_abs;
       
    }
    else if(hdr_back.uid == OAD_APP_STACK_UID) //app and stack
    {
        write_addr = SEC_IMAGE_OAD_HEADER_STACK_FADDR;
        read_end_addr = flash_env.ota_all_backup_oad_header_faddr_abs + backup_size;
        read_addr = flash_env.ota_all_backup_oad_header_faddr_abs;
    }
    //bim_printf("write_addr = %x\r\n",write_addr);
    //bim_printf("read_end_addr = %x\r\n",read_end_addr);
    
    while(read_addr < read_end_addr)
    {
        flash_read(data,read_addr,READ_BLOCK_SIZE);
        flash_write(data,write_addr,READ_BLOCK_SIZE);
        write_addr += READ_BLOCK_SIZE;
        read_addr += READ_BLOCK_SIZE;
    }
    //bim_printf("udi_updata_backup_to_image_sec end\r\n");
}

uint8_t bim_select_sec(void)
{
    uint8_t bsec_status;
    uint8_t status = 0;
    
    bsec_status = bim_check_backup_sec_status();
    bim_printf("bsec_status= %x\n",bsec_status);
    
    switch(bsec_status)
    {
        case SSTATUS_SECT_NORMAL: // 1:I NORMAL ,B NORMAL,updata B -> I,RUN I
        {
            if(GD_25WD80E == flash_mid)
            {
                flash_wp_none();
            }
            else
            {
                if(hdr_back.uid == OAD_APP_PART_UID)
                {
                    flash_wp_128k();
                }
                else if(hdr_back.uid == OAD_APP_STACK_UID) 
                {
                    flash_wp_8k();
                }
            }
            bim_erase_image_sec();
            bim_updata_backup_to_image_sec();
            if(SSTATUS_SECT_NORMAL == bim_check_image_sec_status())
            {
                bim_erase_backup_sec();
                status = 1;
                flash_wp_all();
            }
            else
            {
                flash_wp_all();
                wdt_enable(100);//reset
            }
        }
        break;
        case SSTATUS_SECT_ERASED://://3:I NORMAL,B ERASED,RUN I
        {
            status = 1;
        }
        break;
        case SSTATUS_SECT_ABNORMAL:
        case SSTATUS_SECT_DIFF_ROM_VER:////4:I DIFF_ROM,B ERASED,NOT HAPPEN
        default:
        {
            if(GD_25WD80E == flash_mid)
            {
                flash_wp_none();
            }
            else
            {
                flash_wp_256k();
            }
            status = 1;
            bim_erase_backup_sec();
            flash_wp_all();
        }
        break;
    }
    return status ;
}
uint32_t crc32_table[256];
int make_crc32_table(void)
{
    uint32_t c;
    int i = 0;
    int bit = 0;
    for(i = 0; i < 256; i++)
    {
        c = (uint32_t)i;
        for(bit = 0; bit < 8; bit++)
        {
            if(c&1)
            {
                c = (c>>1)^(0xEDB88320);
            }
            else
            {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
    return 0;
}
uint32_t make_crc32(uint32_t crc,unsigned char *string,uint32_t size)
{
    while(size--)
    {
        crc = (crc >> 8)^(crc32_table[(crc^*string++)&0xff]);
    }
    return crc;
}
