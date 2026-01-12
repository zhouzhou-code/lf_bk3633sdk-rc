/**
 ****************************************************************************************
 *
 * @file app_otah.c
 *
 * @brief otah Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2020.03.17
 *
 * Copyright (C) Beken 2019-2022
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <string.h>
#include "ke_task.h"                 // Kernel
#include "app_otah.h"              // Battery Application Module Definitions
#include "app.h"                   // Application Definitions
#include "app_task.h"              // application task definitions
#include "otahs_task.h"            // health thermometer functions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"             // Profile common types definition
#include "arch.h"                  // Platform Definitions
#include "prf.h"
#include "otahs.h"
#include "ke_timer.h"
#include "uart.h"
#include "prf_utils.h"
#include "user_config.h"
#include "cipher.h"
#include "gcm.h"
#include "flash.h"
/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
#define OAD_APP_PART_UID    (0x42424242)
#define OAD_APP_STACK_UID    (0x53535353)
#define SEC_IMAGE_OAD_HEADER_APP_FADDR                (0x280a0)     

#define SEC_IMAGE_OAD_HEADER_STACK_FADDR            (0x20E0) 


#define SEC_BACKUP_APP_PART_UID_OAD_HEADER_FADDR    (0x52000) //328kb * 1024

#define SEC_BACKUP_APP_STACK_UID_OAD_HEADER_FADDR    (0x40000) //256kb * 1024

/// otah Application Module Environment Structure
struct app_otah_env_tag app_otah_env;
OTAH_SECTION_T otah_bsec;
uint8_t OTAH_reset;
/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
    
void app_otah_init(void)
{

    // Reset the environment
    memset(&app_otah_env, 0, sizeof(struct app_otah_env_tag));
}

void app_otah_add_otahs(void)
{

   struct otahs_db_cfg *db_cfg;
        
   struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct otahs_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl =   0;
    req->prf_task_id = TASK_ID_OTAHS;
    req->app_task = TASK_APP;
    req->start_hdl = 0; //req->start_hdl = 0; dynamically allocated

     
    // Set parameters
    db_cfg = (struct otahs_db_cfg* ) req->param;
   
    db_cfg->otah_nb = 1;
     
    // Sending of notifications is supported
    // Send the message
    ke_msg_send(req);
}


void app_ctrl_send_ind(uint8_t conidx,uint16_t len,uint8_t* buf)
{
    if( ke_state_get(prf_get_task_from_id(TASK_ID_OTAHS))==OTAHS_IDLE )
    {
        struct otahs_env_tag* otahs_env = PRF_ENV_GET(OTAHS, otahs);
        //uart_printf("app_ctrl_send_ind:%d\r\n",len);
        if(otahs_env->ntf_cfg[conidx] == PRF_CLI_START_IND)
        {
            // Allocate the message
            struct otahs_ctrl_val_upd_req * req = KE_MSG_ALLOC(OTAHS_CTRL_VALUE_UPD_REQ,
                                                                prf_get_task_from_id(TASK_ID_OTAHS),
                                                                KE_BUILD_ID(TASK_APP, conidx),
                                                                otahs_ctrl_val_upd_req);
            // Fill in the parameter structure    


            for(int i=0;i<len;i++)
            {
                uart_printf("0x%x ",buf[i]);
            }
            uart_printf("\r\n");
            
            req->length = len;
            memcpy(req->value, buf, len);

            // Send the message
            ke_msg_send(req);
        }
    }
}


static int otahs_ctrl_val_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct otahs_ctrl_val_ntf_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    uart_printf("ctrl->param->ntf_cfg = %x\r\n",param->ntf_cfg);
    if(param->ntf_cfg == PRF_CLI_STOP_NTFIND)
    {
        
    }else
    {
    
    }
    
    return (KE_MSG_CONSUMED);
}


static int ctrl_val_upd_rsp_handler(ke_msg_id_t const msgid,
                                      struct otahs_ctrl_val_upd_rsp const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uart_printf("%s,status:%x\r\n", __func__,param->status);
    
    if(param->status == GAP_ERR_NO_ERROR)
    {
        ke_state_set(dest_id, OTAHS_IDLE);    
    }
    
    return (KE_MSG_CONSUMED);
}

uint32_t otah_write_flash(void)
{
    uint32_t wr_addr;
    uint32_t len ;
    uint8_t i=0;

        
        if(otah_bsec.otah_part == OAD_APP_PART_UID)
            wr_addr = otah_bsec.update_offset + (SEC_BACKUP_APP_PART_UID_OAD_HEADER_FADDR);
        else
            wr_addr = otah_bsec.update_offset + (SEC_BACKUP_APP_STACK_UID_OAD_HEADER_FADDR);

        len = otah_bsec.data_cnt;
        for( i=0;i<len/96;i++)
        {
            flash_write(0, wr_addr+i*96, 96, otah_bsec.data+i*96, NULL);    
        }
        if(len%96 >0)
        {
            flash_write(0, wr_addr+i*96, len%96, otah_bsec.data+i*96, NULL);    
        }
       // flash_write(0, wr_addr, len, sec_ptr->data, NULL);    
        otah_bsec.update_offset += len;
        otah_bsec.data_cnt = 0;        


    return 0; 
}
/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_otah_msg_dflt_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uart_printf("%s\r\n", __func__);
    
    // Drop the message
    return (KE_MSG_CONSUMED);
}


uint8_t file_type;
uint32_t file_size;
uint16_t cur_pkg_num;
uint16_t total_pkg_num;
uint16_t expect_first_index;
uint8_t wait_10_pkg;
uint16_t bitmap_10_pkg=0;
uint8_t hash_buffer[500];
uint16_t hash_buffer_offset;
uint8_t signature_buffer[500];
uint16_t signature_buffer_offset;
uint8_t wait_miss_pkg;
static int data_writer_cmd_handler(ke_msg_id_t const msgid,
                                     struct otahs_data_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t temp_buffer[100]={0};
    uint16_t seg_index;
    

    seg_index=(param->value[0]<<8)|param->value[1];
    uart_printf("len:%d,idx:%d\r\n",param->length,seg_index);
    if(seg_index==0)//first pakge
    {

        file_type = param->value[6];
        file_size = (param->value[7]<<24)|(param->value[8]<<16)|(param->value[9]<<8)|param->value[10];
        total_pkg_num = 1+(file_size-151)/160+((file_size-151)%160>0);
        cur_pkg_num=1;
        uart_printf("file:%d,%d,%d\r\n",file_type,file_size,total_pkg_num);
        switch(file_type)
        {
            case 0: //hash list
            memcpy(hash_buffer,&param->value[11],151);
            hash_buffer_offset=151;
            break;
            case 1://signature
            memcpy(signature_buffer,&param->value[11],151);
            signature_buffer_offset=151;
            break;
            case 2://bin file
            
            wait_10_pkg++;
            memcpy(otah_bsec.data,&param->value[11],151);
            otah_bsec.data_cnt=151;
            otah_bsec.otah_part = co_read32p(&(param->value[19]));
            otah_bsec.update_offset=0;
            expect_first_index = 0;
            bitmap_10_pkg = 0x8000;
            uart_printf("part:%x\r\n",otah_bsec.otah_part);
            break;
        }
        
        
    }else 
    {
        cur_pkg_num++;
        switch(file_type)
        {
            case 0:
                memcpy(hash_buffer +hash_buffer_offset,&param->value[2],160);
                if(cur_pkg_num == total_pkg_num)
                {
                    temp_buffer[0]=0x80;
                    temp_buffer[1]=0x81;
                    temp_buffer[2]=0x00;
                    temp_buffer[3]=0x00;
                    temp_buffer[4]=0xC0;
                    
                    app_ctrl_send_ind(0,12,temp_buffer);//

                }
                break;
            case 1:
                memcpy(signature_buffer +signature_buffer_offset,&param->value[2],160);
                if(cur_pkg_num == total_pkg_num)
                {
                    temp_buffer[0]=0x80;
                    temp_buffer[1]=0x81;
                    temp_buffer[2]=0x00;
                    temp_buffer[3]=0x00;
                    temp_buffer[4]=0xC0;
                    
                    app_ctrl_send_ind(0,12,temp_buffer);//

                }
                break;    
            case 2:
            {
                uint16_t offset;
                if(expect_first_index==0)
                {
                    
                    offset=(seg_index-expect_first_index-1)*160+151;
                }else
                {
                    offset=(seg_index-expect_first_index)*160;
                }
                memcpy(otah_bsec.data+offset,&param->value[2],160);
                otah_bsec.data_cnt=otah_bsec.data_cnt+160;
                bitmap_10_pkg |= 1<<(15+expect_first_index-seg_index);

                uart_printf("WCNT:%x,%x,miss:%x,%x,%x\r\n",bitmap_10_pkg,expect_first_index,wait_miss_pkg,cur_pkg_num,total_pkg_num);
                if((seg_index-expect_first_index)== 9)//本轮第十个包的索引
                {
                    temp_buffer[0]=0x80;
                    temp_buffer[1]=0x81;
                    temp_buffer[2]=expect_first_index>>8;
                    temp_buffer[3]=expect_first_index&0xff;
                    temp_buffer[4]=bitmap_10_pkg>>8;
                    temp_buffer[5]=bitmap_10_pkg&0xff;
                    
                    if(bitmap_10_pkg == 0xffc0) //没丢包，写入flash，开始下一轮
                    {
                        otah_write_flash();
                        bitmap_10_pkg=0;
                        expect_first_index=seg_index+1;
                    }else
                    {
                        wait_miss_pkg=1; //丢包了，等待补包
                    }
                    
                    app_ctrl_send_ind(0,12,temp_buffer);//
                }
                else if((wait_miss_pkg == 1)&&(bitmap_10_pkg == 0xffc0))//本轮缺失包已补齐
                {

                    temp_buffer[0]=0x80;
                    temp_buffer[1]=0x81;
                    temp_buffer[2]=expect_first_index>>8;
                    temp_buffer[3]=expect_first_index&0xff;
                    temp_buffer[4]=bitmap_10_pkg>>8;
                    temp_buffer[5]=bitmap_10_pkg&0xff;
                    otah_write_flash();
                    bitmap_10_pkg=0;
                    expect_first_index=seg_index+1;
                    app_ctrl_send_ind(0,12,temp_buffer);
                }
 
                if(cur_pkg_num == total_pkg_num) //最后一包
                {
                    if(bitmap_10_pkg !=0)
                    {
                        otah_write_flash();
                        
                    }
                    temp_buffer[0]=0x80;
                    temp_buffer[1]=0x03;
                    temp_buffer[2]=0x00;//0 success 1 flash error  2 crc error
                    app_ctrl_send_ind(0,3,temp_buffer);//
                    
                    for(int i = 0 ;i < 3;i++)
                    {
                        uart_printf("wait for reset!!!\r\n");
                    }
                    OTAH_reset =1;
                    ke_timer_set(APP_PERIOD_TIMER,TASK_APP,20);
                    //wdt_enable(500);
                    //while(1);


                }
                break;
             }   
        }
    }

    return (KE_MSG_CONSUMED);

}
    
#define OTAH_CMD_CHKB 0x0001
#define OTAH_CMD_CHKS 0x0002
#define OTAH_CMD_CHKD 0x0004

static int ctrl_writer_cmd_handler(ke_msg_id_t const msgid,
                                     struct otahs_data_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    static uint8_t buf[300];
    
    /*static uint8_t last_msgid=0;
    //static uint16_t buff_offset=0;
    char * temp_str;*/
    
    // Drop the message
    uart_printf("ctrl param->length:%d\r\n",param->length);
    
    for(uint16_t i = 0;i < param->length;i++)
    {
            uart_printf("%2x",param->value[i]);
    }
            uart_printf("\r\n");
    
    if(param->value[1]==OTAH_CMD_CHKB)//check max buffer
    {
        buf[0]=0x80;
        buf[1]=0x01;
        buf[2]=0x0a;
        app_ctrl_send_ind(0,3,buf);

    }else if(param->value[1]==OTAH_CMD_CHKD)//check device info
    {
        extern const char prodId[];
        extern const char *PRODUCT_SN;
        extern const char *firmwareVer;
        char mcu_info[20]={0x10,0x02,0x30,0x00, 0x11,0x02,0x30,0x00, 0x12,0x02
                       ,0x30,0x00, 0x13,0x02,0x30,0x00, 0x14,0x02,0x30,0x00,};
        buf[0]=0x80;
        buf[1]=0x04;
        
        
        buf[2]=0x0; //Module DeviceName (prodid)
        buf[3]=0x9;
        for(uint8_t i=0;i<4;i++)
        {
            if((prodId[i]<=57)&&(prodId[i]>=48))// 数字
            {
                buf[4+2*i]='0';
                buf[5+2*i]=prodId[i];
            }else if((prodId[i]<=90)&&(prodId[i]>=65))//大写字母
            {
                buf[4+2*i]='1';
                buf[5+2*i]=prodId[i]+32;//小写字母
            }else
            {
                buf[4+2*i]='1';
                buf[5+2*i]=prodId[i];//小写字母
            }
        }
        buf[12]=0x0;

        buf[13]=0x1;
        buf[14]=0x9;
        memcpy(buf+15,buf+4,9);//Module SubDeviceName

        buf[24]=0x2;
        buf[25]=0x2;
        buf[26]=0x30;
        buf[27]=0x00; //Module DeviceType
 
        buf[28]=0x3;
        buf[29]=0xd;
        memcpy(buf+30,PRODUCT_SN,0xc);
        buf[42]=0x00;

        buf[43]=0x4;
        buf[44]=strlen(firmwareVer)+1;
        memcpy(buf+45,firmwareVer,strlen(firmwareVer));
        buf[45+strlen(firmwareVer)]=0x0;

        memcpy(buf+46+strlen(firmwareVer),mcu_info,20);//mcu_info
        app_ctrl_send_ind(0,66+strlen(firmwareVer),buf);

    }else if(param->value[1]==OTAH_CMD_CHKS)//check ota head
    {
        uint16_t len=0;
        uint8_t product_id[10];
        uint8_t firmware_ver[10];
        uint32_t firmware_size;
        uint32_t mcu_ver[10];
        uint32_t mcu_size;
        uint8_t * p_value;
        p_value = param->value+2;
        len=2;
        while(len < param->length)
        {
            switch(p_value[0])
            {
                case 0://product id
                    memcpy(product_id, &p_value[2],p_value[1]);
                    break;
                case 1: //firmware ver
                    memcpy(firmware_ver, &p_value[2],p_value[1]);
                    break;
                case 2: //firmware size
                    firmware_size=0;
                    for(char i=0;i<p_value[1];i++)
                    {
                        firmware_size = (firmware_size<<8)|p_value[i+2];
                    }
                    break;
                case 3: //mcu ver
                    memcpy(mcu_ver, &p_value[2],p_value[1]);
                    break;
                case 4: //mcu size
                    mcu_size=0;
                    for(char i=0;i<p_value[1];i++)
                    {
                        mcu_size = (mcu_size<<8)|p_value[i+2];
                    }
                    break;
                    
            }
            len+=p_value[1]+2;
            p_value=p_value+2+p_value[1];
        }
        buf[0]=0x80;
        buf[1]=0x02;
        buf[2]=0x00;
        app_ctrl_send_ind(0,3,buf);
    }
        
            
    return (KE_MSG_CONSUMED);
}



/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_otah_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_otah_msg_dflt_handler},
    {OTAHS_CTRL_VALUE_NTF_CFG_IND,  (ke_msg_func_t)otahs_ctrl_val_ntf_cfg_ind_handler},

    {OTAHS_CTRL_VALUE_UPD_RSP,      (ke_msg_func_t)ctrl_val_upd_rsp_handler},
    {OTAHS_DATA_WRITER_CMD_IND,        (ke_msg_func_t)data_writer_cmd_handler},
    {OTAHS_CTRL_WRITER_CMD_IND,     (ke_msg_func_t)ctrl_writer_cmd_handler},
   
};

const struct app_subtask_handlers app_otah_handler = APP_HANDLERS(app_otah);



