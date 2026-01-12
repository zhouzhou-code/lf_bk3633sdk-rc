/**
 ****************************************************************************************
 *
 * @file app_fcc0.c
 *
 * @brief fcc0 Application Module entry point
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
#include "app_fcc0.h"              // Battery Application Module Definitions
#include "app.h"                   // Application Definitions
#include "app_task.h"              // application task definitions
#include "fcc0s_task.h"            // health thermometer functions
#include "co_bt.h"
#include "co_utils.h"
#include "prf_types.h"             // Profile common types definition
#include "arch.h"                  // Platform Definitions
#include "prf.h"
#include "fcc0s.h"
#include "ke_timer.h"
#include "uart.h"
#include "prf_utils.h"
#include "user_config.h"
#include "cipher.h"
#include "gcm.h"
#include "md.h"

/*
 * DEFINES
 ****************************************************************************************
 */
/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
extern uint16_t my_strstr( uint8_t *str_src, const char *str_dst, uint16_t src_len,uint16_t dst_len);
extern void hex_to_str(uint8_t *hex_buffer,char *char_buffer ,uint16_t hex_len);
extern uint16_t set_deviceinfo_cmdrsp(uint8_t *data_buffer);
extern void str_to_hex(uint8_t *data_buffer, char *str, uint16_t str_len );
extern void comput_session_key(void);
extern void comput_hmac_key(void);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// fcc0 Application Module Environment Structure
struct app_fcc0_env_tag app_fcc0_env;

extern uint8_t sess_sn1[8];
uint8_t sess_seq[6];
uint8_t sess_uuid[38];
uint8_t sess_id[32]={0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,
    0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20};
extern uint8_t sess_sn2[8];

extern uint8_t authCode[16];
extern uint8_t authCodeId[16];
    
void app_fcc0_init(void)
{

    // Reset the environment
    memset(&app_fcc0_env, 0, sizeof(struct app_fcc0_env_tag));
}

void app_fcc0_add_fcc0s(void)
{

   struct fcc0s_db_cfg *db_cfg;
        
   struct gapm_profile_task_add_cmd *req = KE_MSG_ALLOC_DYN(GAPM_PROFILE_TASK_ADD_CMD,
                                                  TASK_GAPM, TASK_APP,
                                                  gapm_profile_task_add_cmd, sizeof(struct fcc0s_db_cfg));
    // Fill message
    req->operation = GAPM_PROFILE_TASK_ADD;
    req->sec_lvl =   0;
    req->prf_task_id = TASK_ID_FCC0S;
    req->app_task = TASK_APP;
    req->start_hdl = 0; //req->start_hdl = 0; dynamically allocated

     
    // Set parameters
    db_cfg = (struct fcc0s_db_cfg* ) req->param;
   
    db_cfg->fcc0_nb = 1;
     
    // Sending of notifications is supported
    // Send the message
    ke_msg_send(req);
}


void app_fcc2_send_ind(uint8_t conidx,uint16_t len,uint8_t* buf)
{
    if( ke_state_get(prf_get_task_from_id(TASK_ID_FCC0S))==FCC0S_IDLE )
    {
        struct fcc0s_env_tag* fcc0s_env = PRF_ENV_GET(FCC0S, fcc0s);
        uart_printf("app_fcc2_send_ind:%d\r\n",len);
        if(fcc0s_env->ntf_cfg[conidx] == PRF_CLI_START_IND)
        {
            // Allocate the message
            struct fcc0s_fcc2_val_upd_req * req = KE_MSG_ALLOC(FCC0S_FCC2_VALUE_UPD_REQ,
                                                                prf_get_task_from_id(TASK_ID_FCC0S),
                                                                KE_BUILD_ID(TASK_APP, conidx),
                                                                fcc0s_fcc2_val_upd_req);
            // Fill in the parameter structure    


            for(int i=0;i<len;i++)
            {
                uart_printf("%c",buf[i]);
            }
            uart_printf("\r\n");
            
            req->length = len;
            memcpy(req->value, buf, len);

            // Send the message
            ke_msg_send(req);
        }
    }
}


static int fcc0s_fcc2_val_ntf_cfg_ind_handler(ke_msg_id_t const msgid,
                                               struct fcc0s_fcc2_val_ntf_cfg_ind const *param,
                                               ke_task_id_t const dest_id,
                                               ke_task_id_t const src_id)
{
    uart_printf("fcc2->param->ntf_cfg = %x\r\n",param->ntf_cfg);
    if(param->ntf_cfg == PRF_CLI_STOP_NTFIND)
    {
        
    }else
    {
    
    }
    
    return (KE_MSG_CONSUMED);
}


static int fcc2_val_upd_rsp_handler(ke_msg_id_t const msgid,
                                      struct fcc0s_fcc2_val_upd_rsp const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uart_printf("%s,status:%x\r\n", __func__,param->status);
    
    if(param->status == GAP_ERR_NO_ERROR)
    {
        ke_state_set(dest_id, FCC0S_IDLE);    
    }
    
    return (KE_MSG_CONSUMED);
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
static int app_fcc0_msg_dflt_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uart_printf("%s\r\n", __func__);
    
    // Drop the message
    return (KE_MSG_CONSUMED);
}

void hilink_cmd_parase(uint8_t * data_buffer, uint16_t buf_len)
{
    char * temp_str;
    
    temp_str = "netCfgVer";
    my_strstr(data_buffer,temp_str,buf_len,strlen(temp_str));
    

}

uint16_t set_createsession_cmdrsp(uint8_t *data_buffer)
{
    uint16_t data_len=0;
    char * temp_str;
    char  str_from_hex[64];
    
    temp_str="{\"seq\":";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,sess_seq,5);
    data_len=data_len+5;

    temp_str =",\"uuid\": \"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,sess_uuid,36);
    data_len=data_len+36;


    hex_to_str(sess_id,str_from_hex,32);
    temp_str ="\",\"sessId\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,str_from_hex,64);
    data_len=data_len+64; 

    hex_to_str(sess_sn2,str_from_hex,8);
    temp_str = "\",\"sn2\": \"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,str_from_hex,16);
    data_len=data_len+16; 

    hex_to_str(authCodeId,str_from_hex,16);
    temp_str = "\",\"authCodeId\": \"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,str_from_hex,32);
    data_len=data_len+32; 

    data_buffer[data_len++]='"';
    data_buffer[data_len++]='}';   

    
    return data_len;
}


#define AES128_KEY_LENGTH 128
#define GCM_TAG_LEN 16 
#define GCM_IV_LEN 12

unsigned char tag[GCM_TAG_LEN] = {0}; 
uint8_t IV_code[12] = {0x62,0x30,0xfc,0x88,0x79,0x87,0x4d,0xde,0x59,0x53,0xb9,0xdc};

//uint8_t tmp_test_sessionkey[16] = {0x4b,0x2f,0x68,0x3b,0xa3,0x6e,0x8e,0xfa,0x4d,0xb2,0x0e,0xf2,0x5b,0xc2,0x9e,0x4b};
extern uint8_t sessionKey[16];
extern const char prodId[];

void encrypt_sec_data(uint8_t *input, uint8_t *output, uint16_t inputLen)
{
    //memcpy(sessionKey,tmp_test_sessionkey,sizeof(sessionKey));
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context); //设置加密使用的sessionKey。 
    mbedtls_gcm_setkey(&context, 
        MBEDTLS_CIPHER_ID_AES, sessionKey, AES128_KEY_LENGTH); 
    mbedtls_gcm_crypt_and_tag(&context,  MBEDTLS_GCM_ENCRYPT,  inputLen, //需要加密数据的长度
                        IV_code, //设备侧生成12位随机数,拼包时放在body的前12Byte  
                        GCM_IV_LEN, //IV长度 12Byte  
                        (const unsigned char *)prodId, //productId，字符串    
                        4, //productId长度，值为4
                        input, //需要加密的数据
                        output, //加密后的数据
                        GCM_TAG_LEN, //tag长度 16Byte
                        tag); //tag，拼接在加密数据后面 
    memcpy(output + inputLen,  tag, GCM_TAG_LEN); //将tag添加到数据尾部


//    uart_printf("encrypt_custom_data:\r\n");
//    for(i=0;i<inputLen+GCM_TAG_LEN;i++)
//        uart_printf("0x%x ",(uint8_t)output[i]);
//    uart_printf("\r\n");
}

void decrypt_sec_data(uint8_t * inbuffer,uint16_t buf_len,uint8_t * out_data)
{
    uint8_t IV_data[12];
    uint8_t tag_data[16];
    mbedtls_gcm_context context;
    extern uint8_t sessionKey[16];
    memcpy(IV_data,inbuffer,12);
    memcpy(tag_data,inbuffer+(buf_len-48),16);
    mbedtls_gcm_init(&context);
    mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, sessionKey, AES128_KEY_LENGTH);

    mbedtls_gcm_auth_decrypt(&context,buf_len - 60, IV_data, 12,  (const unsigned char *)prodId, \
            4, tag_data, 16, inbuffer+12, out_data); //?a?üê?3?μ?êy?Y



    
    uart_printf("decrydata:\r\n");
    for(int i=0;i<(buf_len - 60);i++)
    {
        uart_printf("%c",out_data[i]);


    }
    uart_printf("\r\n");

}

uint8_t cmd_netcfgver[11]="netCfgVer";
uint8_t cmd_deviceinfo[12]="deviceInfo";
int8_t device_ver[]={ '{','"','v','e','r','"',':','1','0','0','}'};
static int fcc1_writer_cmd_handler(ke_msg_id_t const msgid,
                                     struct fcc0s_fcc1_writer_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    static uint8_t buf[300];
    //static uint8_t last_msgid=0;
    static uint16_t buff_offset=0;
    extern uint8_t authCode[16];
    extern uint8_t authCodeId[16];
    char * temp_str;
    // Drop the message
    uart_printf("param->length:%d\r\n",param->length);
    
    for(uint16_t i = 0;i < param->length;i++)
    {
            uart_printf("%2x",param->value[i]);
    }
            uart_printf("\r\n");
    
    if(param->value[2]==1)//one pgk
    {
        if(!memcmp(&param->value[9],cmd_netcfgver,param->value[8]))//0 34 1 0 0 0 0 11 9 6e 65 74 43 66 67 56 65 72 0 0
        {

            memcpy(buf,param->value,param->value[8]+9);
            buf[0]=1;
            buf[param->value[8]+9]=sizeof(device_ver)&0xff;
            buf[param->value[8]+10]=sizeof(device_ver)>>8;
            memcpy(&buf[param->value[8]+11],device_ver,sizeof(device_ver));
            app_fcc2_send_ind(param->conidx,param->value[8]+11+sizeof(device_ver),buf);
            
        }else if(!memcmp(&param->value[9],cmd_deviceinfo,param->value[8]))
        {
            uint16_t buf_len;
            
            memcpy(buf,param->value,param->value[8]+9);
            buf[0]=1;
            buf_len = set_deviceinfo_cmdrsp(&buf[param->value[8]+11]);
            buf[param->value[8]+9] =buf_len&0xff;
            buf[param->value[8]+10]=buf_len>>8;
            
            app_fcc2_send_ind(param->conidx,param->value[8]+11+buf_len,buf);
            
        }
    }else//multi pkg
    {
        
        if(param->value[3]==0)  //first pkg
        {
            memset(buf,0,sizeof(buf));
            memcpy(buf,param->value,param->length);
            buff_offset=param->length;
        }else
        {
            memcpy(buf+buff_offset,&param->value[7],param->length-7);
            buff_offset=buff_offset+param->length-7;
        }
        
        uart_printf("pkg_num=%d\r\n",param->value[3]);
        
        if((param->value[2]-1)==param->value[3])//last pkg
        {
             
            for(uint16_t i = 0;i < buff_offset;i++)
            {
                uart_printf("%2x",buf[i]);
            }
    
            uart_printf("\r\n");
            
            temp_str="authSetup";
            //uart_printf("offset=%d,%d\r\n",buff_offset,my_strstr(buf,temp_str,buff_offset,strlen(temp_str)));
            
            if(my_strstr(buf,temp_str,buff_offset,strlen(temp_str)))
            {
                uint16_t end_bit;
                uint8_t rsp_buffer[60];
                
                temp_str="\"authCode\":\"";
                end_bit=my_strstr(buf,temp_str,buff_offset,strlen(temp_str));
                uart_printf("1111=%d\r\n",end_bit);
                str_to_hex(authCode,(char *)&buf[end_bit],32);


                temp_str="\"authCodeId\":\"";
                end_bit=my_strstr(buf,temp_str,buff_offset,strlen(temp_str));
                uart_printf("authCode:\r\n",end_bit);
                str_to_hex(authCodeId,(char *)&buf[end_bit],32);

                for(uint8_t i=0;i<16;i++)
                {
                    uart_printf("0x%x ",authCode[i]);
                }
                uart_printf("\r\nauthCodeId: ");
                for(uint8_t i=0;i<16;i++)
                {
                    uart_printf("0x%x ",authCodeId[i]);
                }
                uart_printf("\r\n");


                temp_str="{\"errcode\":\"0\"}";
                memcpy(rsp_buffer,buf,buf[8]+9);
                rsp_buffer[0]=1;
                rsp_buffer[2]=1;
                rsp_buffer[buf[8]+9] =strlen(temp_str)&0xff;
                rsp_buffer[buf[8]+10]=strlen(temp_str)>>8;
                memcpy(&rsp_buffer[buf[8]+11],temp_str,strlen(temp_str));
                app_fcc2_send_ind(param->conidx,buf[8]+11+strlen(temp_str),rsp_buffer);

            }
            temp_str="createSession";
            if(my_strstr(buf,temp_str,buff_offset,strlen(temp_str)))
            {
                 uint16_t end_bit;
                 uint8_t rsp_buffer[300];
                 uint16_t resp_len;
                 temp_str="\"sn1\":\"";
                 end_bit=my_strstr(buf,temp_str,buff_offset,strlen(temp_str));
                 str_to_hex(sess_sn1,(char *)&buf[end_bit],16);

                 temp_str="\"seq\":";
                 end_bit=my_strstr(buf,temp_str,buff_offset,strlen(temp_str));
                 memcpy(sess_seq,&buf[end_bit],5);
                 
                 temp_str="\"uuid\":\"";
                 end_bit=my_strstr(buf,temp_str,buff_offset,strlen(temp_str));
                 memcpy(sess_uuid,&buf[end_bit],36);

                memcpy(rsp_buffer,buf,buf[8]+9);
                rsp_buffer[0]=1;
                rsp_buffer[2]=1;
                resp_len = set_createsession_cmdrsp(&rsp_buffer[buf[8]+11]);
                rsp_buffer[buf[8]+9] =resp_len&0xff;
                rsp_buffer[buf[8]+10]=resp_len>>8;
                app_fcc2_send_ind(param->conidx,buf[8]+11+resp_len,rsp_buffer);

                comput_session_key(); 
                comput_hmac_key();
            }
            temp_str = "customSecData";
            if(my_strstr(buf,temp_str,buff_offset,strlen(temp_str)))
            {
                 //uint16_t end_bit;
                 //uint16_t secdata_len;
                 uint8_t out_buffer[300];
                 //uint16_t out_len;
                 uint8_t rsp_buffer[300];
                 //uint16_t resp_len;
                 uint8_t mac_buff[32];
                 const mbedtls_md_info_t *mdInfo = NULL;
                 extern uint8_t hmacKey[32];

                // secdata_len = (buf[buf[8]+10]<<8)|buf[buf[8]+9];
                // out_len = decrypt_sec_data(&buf[buf[8]+11],secdata_len,out_buffer);

                 memcpy(out_buffer,buf,7);
                 out_buffer[0]=1;
                 out_buffer[2]=1;
                 
                 rsp_buffer[0]=0x11;
                 rsp_buffer[1]=0x0d;
                 temp_str = "customSecData";
                 memcpy(rsp_buffer+2,temp_str,0xd);
                 temp_str ="{\"errcode\": 0 }";
                 encrypt_sec_data((uint8_t *)temp_str,rsp_buffer+0x11,strlen(temp_str));
                 rsp_buffer[0xf]=strlen(temp_str)+16;
                 rsp_buffer[0x10]=0x0;
                 memcpy(out_buffer+7,rsp_buffer,rsp_buffer[0xf]+0x11);
                
                
                 mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
                 mbedtls_md_hmac(mdInfo,
                  hmacKey, //hmacKey。
                  32, //hmacKey 长度，32Byte
                  rsp_buffer, //打包的 payload 数据
                  rsp_buffer[0xf]+0x11, //payload 数据
                  mac_buff //Hmac 校验值
                 ); 

                 memcpy(out_buffer+rsp_buffer[0xf]+0x18,mac_buff,32);

                 
                 app_fcc2_send_ind(param->conidx,rsp_buffer[0xf]+0x18+32,out_buffer);
                 
            }
        }
        else
        {
            uint8_t rsp_buffer[2];
            rsp_buffer[0]=0xff;
            if(param->value[1] == 1)
            {
                rsp_buffer[1]=1;
            }
            else
            {
                rsp_buffer[1]=0;
            }
            
            app_fcc2_send_ind(param->conidx,2,rsp_buffer);
        }
    }
    
            
    return (KE_MSG_CONSUMED);
}



/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Default State handlers definition
const struct ke_msg_handler app_fcc0_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,        (ke_msg_func_t)app_fcc0_msg_dflt_handler},
    {FCC0S_FCC2_VALUE_NTF_CFG_IND,  (ke_msg_func_t)fcc0s_fcc2_val_ntf_cfg_ind_handler},

    {FCC0S_FCC2_VALUE_UPD_RSP,      (ke_msg_func_t)fcc2_val_upd_rsp_handler},
    {FCC0S_FCC1_WRITER_CMD_IND,        (ke_msg_func_t)fcc1_writer_cmd_handler},
   
};

const struct app_subtask_handlers app_fcc0_handler = APP_HANDLERS(app_fcc0);



