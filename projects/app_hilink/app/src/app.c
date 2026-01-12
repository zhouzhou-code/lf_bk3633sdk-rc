/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2015
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

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "user_config.h"  

#include "rwip_config.h"             // SW configuration
#include "md.h"
#include "ke_mem.h"
#include "rwble_config.h"

#if (BLE_APP_PRESENT)
#include "rwapp_config.h"
#include <string.h>

#include "rwip.h"
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "gap.h"                     // GAP Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API

#include "co_bt.h"                   // Common BT Definition
#include "co_math.h"                 // Common Maths Definition
#include "pkcs5.h"

#if (BLE_APP_SEC)
#include "app_sec.h"                 // Application security Definition
#endif // (BLE_APP_SEC)

#if (BLE_APP_FEE0S)
#include "app_fee0.h"                  // USER define FEE0S Application Definitions
#endif //(BLE_APP_FEE0S)

#if (BLE_APP_FCC0S)
#include "app_fcc0.h"                  // USER define FCC0S Application Definitions
#endif //(BLE_APP_FCC0S)

#if(BLE_APP_ANCS)
#include "app_ancsc.h"               // Application ancs Definition
#endif
#if(BLE_APP_OADS)
#include "app_oads.h"                 // Application oads Definition
#endif

#if(BLE_APP_OTAHS)
#include "app_otah.h"                 // Application oads Definition
#endif

#if (BLE_APP_HT)
#include "app_ht.h"                  // Health Thermometer Application Definitions
#endif //(BLE_APP_HT)

#if (BLE_APP_DIS)
#include "app_dis.h"                 // Device Information Service Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_BATT)
#include "app_batt.h"                // Battery Application Definitions
#endif //(BLE_APP_DIS)

#if (BLE_APP_HID)
#include "app_hid.h"                 // HID Application Definitions
#endif //(BLE_APP_HID)

#if (DISPLAY_SUPPORT)
#include "app_display.h"             // Application Display Definition
#endif //(DISPLAY_SUPPORT)

#if (BLE_APP_AM0)
#include "app_am0.h"                 // Audio Mode 0 Application
#endif //(BLE_APP_AM0)

#if (NVDS_SUPPORT)
#include "nvds.h"                    // NVDS Definitions
#endif //(NVDS_SUPPORT)
#include "flash.h"  
#include "app_hid.h"
/*
 * DEFINES
 ****************************************************************************************
 */

#if (BLE_APP_HID)
// HID Mouse
#define DEVICE_NAME        "Hid Mouse"
#else
#define DEVICE_NAME        "BK DEVICE"
#endif

#define DEVICE_NAME_SIZE    sizeof(DEVICE_NAME)

/**
 * UUID List part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x03 - Complete list of 16-bit UUIDs available
 * x09\x18 - Health Thermometer Service UUID
 *   or
 * x12\x18 - HID Service UUID
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_UUID        "\x03\x03\x09\x18"
#define APP_HT_ADV_DATA_UUID_LEN    (4)
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_UUID       "\x03\x03\x12\x18"
#define APP_HID_ADV_DATA_UUID_LEN   (4)
#endif //(BLE_APP_HID)

/**
 * Appearance part of ADV Data
 * --------------------------------------------------------------------------------------
 * x03 - Length
 * x19 - Appearance
 * x03\x00 - Generic Thermometer
 *   or
 * xC2\x04 - HID Mouse
 * --------------------------------------------------------------------------------------
 */

#if (BLE_APP_HT)
#define APP_HT_ADV_DATA_APPEARANCE    "\x03\x19\x00\x03"
#endif //(BLE_APP_HT)

#if (BLE_APP_HID)
#define APP_HID_ADV_DATA_APPEARANCE   "\x03\x19\xC1\x03"
#endif //(BLE_APP_HID)

#define APP_ADV_DATA_APPEARANCE_LEN  (4)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef void (*appm_add_svc_func_t)(void);

extern struct bd_addr co_default_bdaddr;

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// List of service to add in the database
enum appm_svc_list
{
    #if (BLE_APP_FEE0S)
    APPM_SVC_FEE0S,
    #endif //(BLE_APP_FEE0S)
        
    #if (BLE_APP_HT)
    APPM_SVC_HTS,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    APPM_SVC_DIS,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    APPM_SVC_BATT,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    APPM_SVC_HIDS,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_AM0)
    APPM_SVC_AM0_HAS,
    #endif //(BLE_APP_AM0)

    #if(BLE_APP_ANCS)
    APPM_SVC_ANCSC,
    #endif
    
    #if(BLE_APP_OADS)
    APPM_SVC_OADS,
    #endif
    
    #if (BLE_APP_FCC0S)
    APPM_SVC_FCC0S,
    #endif //(BLE_APP_FCC0S)
    
    #if(BLE_APP_OTAHS)
    APPM_SVC_OTAHS,
    #endif

    APPM_SVC_LIST_STOP,
};

/*
 * LOCAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
struct DevInfo demoDevInfo={

"3633",
"00",
"0123456789012345",


};


#if 1
#define PRODUCT_ID_0 '2'//'9'
#define PRODUCT_ID_1 'H'//'F'
#define PRODUCT_ID_2 'M'//'Y'
#define PRODUCT_ID_3 'Y'//'8'

 const char brandEn[] = {"BEKEN"};             //kitfwk认证关键项，与品牌英文名一致
 /* 设备型号 */
 const char *deviceModel = "bk-b002";
 const char deviceName[] = {"BK3633-DEMO02"};//与产品名称一致
 /* 设备类型ID */
 const char *deviceTypeId = "0DE";
 /* 设备类型英文名称 */
 const char *deviceTypeNameEn = "Car Diffuser";
 /* 设备固件版本号 */
 const char *firmwareVer = "1.0.0";
 /* 设备软件版本号 */
 const char *softwareVer = "bk_sw_2.0.0";    //厂商软件型号
 /* 设备硬件版本号 */
 const char *hardwareVer = "bk_hw_2.0.0";    //厂商硬件型号
 /* 设备制造商ID */
 const char *manufacturerId = "fhc";
 /* 设备制造商英文名称 */
 const char *manufacturerNameEn = "BEKEN";
 /* 设备产品ID */
 const char prodId[] = {PRODUCT_ID_0,PRODUCT_ID_1,PRODUCT_ID_2,PRODUCT_ID_3,0};
 /* 设备产品key */
 const char *prodKey = "da3409ab3b9a4cb09bc495d9872480eb";

 /* 设备产品子型号ID */
 const char *SUB_PRODUCT_ID = "";
 const char productSeries[] = {"Car Diffuser"};         //与序列一致


 /* AC参数 */
 unsigned char Ackey[48] = {
    0x24, 0x3D, 0x4C, 0x77, 0x3B, 0x37, 0x6C, 0x70, 
    0x66, 0x60, 0x2F, 0x4C, 0x74, 0x29, 0x2F, 0x24,
    0xD8, 0x01, 0x68, 0xB9, 0xF1, 0x7E, 0x6D, 0xC9,
    0x07, 0xAD, 0x38, 0xF1, 0x28, 0x1A, 0x6E, 0x67,
    0x15, 0x3C, 0xAB, 0xCC, 0x26, 0xBA, 0xC1, 0xA7,
    0x2C, 0x63, 0x7E, 0x9F, 0x4E, 0xF1, 0x7F, 0xBD
};


const char *OHOS_SERIAL = "123456789";        // //kitfwk认证关键项，代表产品唯一标识，不应该由宏定义，应该由厂家产线烧录预置到flash，通过修改char* HalGetSerial(void)函数，返回flash中产线预置的这个唯一标识，调试时如果临时使用此宏，注意保证相同型号此编号唯一，如果重复相同型号只能添加一台设备。

const char OHOS_PRODUCT_TYPE[] = {"linkiot"};    //固定
const char OHOS_HARDWARE_PROFILE[] = {"{RAM:256,FLASH:256,WIFI:true,BLE:true}"}; //根据芯片确定
const char OHOS_BOOTLOADER_VERSION[] = {"bootloader_bk72xx_uart2_v1.0.8"};    //调试无影响，认证前确认
const char OHOS_SECURITY_PATCH_TAG[] = {"2022-05-01"};     //调试无影响，认证前确认
const char OHOS_ABI_LIST[] = {"riscv-liteos"};        // 固定值
const char mac_str[17];
/* 设备SN */
 const char *PRODUCT_SN = "123456781111";



//AAA，由产品品牌名与设备名称组成，伙伴自定义，1~10 位。
uint8_t myadvData_unreg[] = {
0x2, 0x1, 0x6
 };
uint8_t myrspData_unreg[] = {
0x16, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6','3','3',0x2d, 0x31,    
    
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
'0','0',
'1','1','1','1',
 };
uint8_t myadvData_reg[] = {
0x2, 0x1, 0x6
 };
uint8_t myrspData_reg[] = {
0x17, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6','3','3',0x2d, 0x31,   
    
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
'1','1','1','1',
2,10,0
 };
uint8_t myadvData_sec[30] = {
0x2, 0x1, 0x6, 0x17, 0x16, 0xEE, 0xFD, 0x01, 0x01, 0x0D, 0x04, 0x00, 0x11, 0xC8,
0x12,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
0xFF, 0x17, 0x01, 0x01, 0x14, 0x02, 0x31, 0x31,
//0x91, 0x01, 0x01
 }; 
uint8_t myrspData_sec[] = {    
    
0x14, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6', '3', '3', 0x2d, 0x31,
//0x14-3, 0x9, 'H', 'i', '-', 'O', 'P', 'L' ,0x2d, 0x31,

PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
'1','1','1','1',

0x02,0x3C,0x00

 };
uint8_t myadvData_first[30] = {
0x2, 0x1, 0x6, 0x17, 0x16, 0xEE, 0xFD, 0x01, 0x01, 0x0D, 0x04, 0x00, 0x11, 0xC8,
0x12,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
0xFF, 0x17, 0x01, 0x15, 0x14, 0x02, 0x31, 0x31,
//0x91, 0x01, 0x01
 }; 
uint8_t myrspData_first[] = {
    
0x16, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6', '3', '3', 0x2d, 0x31,
//0x16-3, 0x9, 'H', 'i', '-', 'O', 'P', 'L', 0x2d, 0x31,
    
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
'0','0',
'1','1','1','1',
 };
#else
#define PRODUCT_ID_0 '2'
#define PRODUCT_ID_1 'I'
#define PRODUCT_ID_2 'C'
#define PRODUCT_ID_3 'J'
 const char brandEn[] = {"belon"};             //kitfwk认证关键项，与品牌英文名一致
 /* 设备型号 */
 const char *deviceModel = "BLCUP_01";
 const char deviceName[] = {"BLCUP"};//与产品名称一致
 /* 设备类型ID */
 const char *deviceTypeId = "06B";
 /* 设备类型英文名称 */
 const char *deviceTypeNameEn = "smart cup";
 /* 设备固件版本号 */
 const char *firmwareVer = "1.0.0";
 /* 设备软件版本号 */
 const char *softwareVer = "bk_sw_2.0.0";    //厂商软件型号 "1.0.0";
 /* 设备硬件版本号 */
 const char *hardwareVer = "1.0.0";    //厂商硬件型号
 /* 设备制造商ID */
 const char *manufacturerId = "0ed";
 /* 设备制造商英文名称 */
 const char *manufacturerNameEn = "Belon ";
 /* 设备产品ID */
 const char prodId[] = {PRODUCT_ID_0,PRODUCT_ID_1,PRODUCT_ID_2,PRODUCT_ID_3,0};
 /* 设备产品key */
 const char *prodKey = "6b4fcfd604154f21a937ee1ca861eada";

 /* 设备产品子型号ID */
 const char *SUB_PRODUCT_ID = "";
 const char productSeries[] = {"BLCUP666"};         //与序列一致

// 7B3466764530 4A774C542043 30705164 51FAC8B98BF1E276 7D52BEA9 7AFADD05 F4B70404FD122FD6 73E5857348C1586C
 /* AC参数 */
 unsigned char Ackey[48] = {
    0x7b, 0x34, 0x66, 0x76, 0x45, 0x30, 0x4A, 0x77, 
    0x4C, 0x54, 0x20, 0x43, 0x30, 0x70, 0x51, 0x64,
    0x51, 0xFA, 0xC8, 0xB9, 0x8B, 0xF1, 0xE2, 0x76,
    0x7D, 0x52, 0xBE, 0xA9, 0x7A, 0xFA, 0xDD, 0x05,
    0xF4, 0xB7, 0x04, 0x04, 0xFD, 0x12, 0x2F, 0xD6,
    0x73, 0xE5, 0x85, 0x73, 0x48, 0xC1, 0x58, 0x6C
};

const char *OHOS_SERIAL = "123456789";        // //kitfwk认证关键项，代表产品唯一标识，不应该由宏定义，应该由厂家产线烧录预置到flash，通过修改char* HalGetSerial(void)函数，返回flash中产线预置的这个唯一标识，调试时如果临时使用此宏，注意保证相同型号此编号唯一，如果重复相同型号只能添加一台设备。

const char OHOS_PRODUCT_TYPE[] = {"linkiot"};    //固定
const char OHOS_HARDWARE_PROFILE[] = {"{RAM:256,FLASH:256,WIFI:true,BLE:true}"}; //根据芯片确定
const char OHOS_BOOTLOADER_VERSION[] = {"bootloader_bk72xx_uart2_v1.0.8"};    //调试无影响，认证前确认
const char OHOS_SECURITY_PATCH_TAG[] = {"2022-05-01"};     //调试无影响，认证前确认
const char OHOS_ABI_LIST[] = {"riscv-liteos"};        // 固定值
const char mac_str[17];
/* 设备SN */
const char *PRODUCT_SN = "123456781111";
#if 0
uint8_t authCode[16]={0xb5,0x50,0x08,0x67,0x50,0x03,0x05,0xed,0x92,0xbb,0xb4,0x68,0xcd,0xc6,0x23,0x6d};
uint8_t authCodeId[16];
uint8_t sessionKey[16];
uint8_t hmacKey[32];

uint8_t sess_sn1[8]={0x85,0x79,0xea,0xa6,0x3c,0xe5,0x69,0x75};
uint8_t sess_sn2[8]={0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38};
#endif
//AAA，由产品品牌名与设备名称组成，伙伴自定义，1~10 位。
uint8_t myadvData_unreg[] = {
0x2, 0x1, 0x6
 };

uint8_t myrspData_unreg[] = {
// 0x16, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6','3','3',0x2d, 0x31,  
0x17, 0x9, 'H', 'i', '-', 'T', 'E', 'S', 'T','C','U','P',0x2d, 0x31,    
//0x16-3, 0x9, 'H', 'i', '-', 'O', 'P', 'L' ,0x2d, 0x31,
    
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
'0','0',
'1','1','1','1',
//'2','3','4','5',
 };

uint8_t myadvData_reg[] = {
0x2, 0x1, 0x6
 };

uint8_t myrspData_reg[] = {
// 0x17, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6','3','3',0x2d, 0x31,
0x18, 0x9, 'H', 'i', '-', 'T', 'E', 'S', 'T','C','U','P',0x2d, 0x31,     
//0x17-3, 0x9, 'H', 'i', '-', 'O', 'P', 'L' ,0x2d, 0x31,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
 '1','1','1','1',
//'2','3','4','5',
2,10,0
};

uint8_t myadvData_sec[31] = {
0x2, 0x1, 0x6, 0x17, 0x16, 0xEE, 0xFD, 0x01, 0x01, 0x0D, 0x04, 0x00, 0x11, 0xC8,
0x12,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
0xFF, 0x17, 0x01, 0x01, 0x14, 0x02, 0x31, 0x31,
//0x91, 0x01, 0x01
 }; 
uint8_t myrspData_sec[] = {
    
// 0x14, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6', '3', '3', 0x2d, 0x31,
0x15, 0x9, 'H', 'i', '-', 'T', 'E', 'S', 'T','C','U','P',0x2d, 0x31, 
//0x14-3, 0x9, 'H', 'i', '-', 'O', 'P', 'L' ,0x2d, 0x31,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
 '1','1','1','1',
//'2','3','4','5',
0x02,0x3C,0x00
 };

uint8_t myadvData_first[31] = {
0x2, 0x1, 0x6, 0x17, 0x16, 0xEE, 0xFD, 0x01, 0x01, 0x0D, 0x04, 0x00, 0x11, 0xC8,
0x12,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
0xFF, 0x17, 0x01, 0x15, 0x14, 0x02, 0x31, 0x31,
//0x91, 0x01, 0x01
 }; 
uint8_t myrspData_first[] = {
    
// 0x16, 0x9, 'H', 'i', '-', 'B', 'K', '3', '6', '3', '3', 0x2d, 0x31,
0x17, 0x9, 'H', 'i', '-', 'T', 'E', 'S', 'T','C','U','P',0x2d, 0x31, 
//0x16-3, 0x9, 'H', 'i', '-', 'O', 'P', 'L', 0x2d, 0x31,
PRODUCT_ID_0, PRODUCT_ID_1, PRODUCT_ID_2,PRODUCT_ID_3,
'0','0',
 '1','1','1','1',
//'2','3','4','5',
 };
#endif

uint8_t authCode[16]={0xb5,0x50,0x08,0x67,0x50,0x03,0x05,0xed,0x92,0xbb,0xb4,0x68,0xcd,0xc6,0x23,0x6d};
uint8_t authCodeId[16];
uint8_t sessionKey[16];
uint8_t hmacKey[32];

uint8_t sess_sn1[8]={0x85,0x79,0xea,0xa6,0x3c,0xe5,0x69,0x75};
uint8_t sess_sn2[8]={0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38};
void mac_hex_to_str(uint8_t *data_buffer,struct bd_addr addr )
{
    char all_char[16]="0123456789ABCDEF"; 

    for(char i=0;i<6;i++)
    {
        data_buffer[3*i+0]=all_char[addr.addr[5-i]>>4];
        data_buffer[3*i+1]=all_char[addr.addr[5-i]&0xf];
        
        if(i!=5)
           data_buffer[3*i+2]=':';
    }

}


void hex_to_str(uint8_t *hex_buffer,char *char_buffer ,uint16_t hex_len)
{
    char all_char[16]="0123456789abcdef";
    
    for(uint16_t i=0;i<hex_len;i++)
    {
        for(uint8_t j=0;j<16;j++)
        {
            if(j == (hex_buffer[i]>>4))
            {
                char_buffer[2*i]=all_char[j];
                break;
            }
        }

        for(uint8_t j=0;j<16;j++)
        {
            if(j == (hex_buffer[i]&0xf))
            {
                char_buffer[2*i+1]=all_char[j];
                break;
            }
        }


    }
    for(int i=0;i< hex_len*2;i++)
    {
        uart_printf("%c",char_buffer[i]);

    }
}

void str_to_hex(uint8_t *data_buffer, char *str, uint16_t str_len )
{
    char all_char[16]="0123456789abcdef";
    char all_char_A[16]="0123456789ABCDEF";
    uint8_t h_bit,l_bit;
    for(uint16_t i=0;i<str_len;i=i+2)
    {
        for(uint8_t j=0;j<16;j++)
        {
            if((all_char[j]==str[i]) || (all_char_A[j]==str[i]))
            {
                h_bit = j;
                break;
            }
        }
        
        for(uint8_t j=0;j<16;j++)
        {
            if((all_char[j]==str[i+1]) || (all_char_A[j]==str[i+1]))
            {
                l_bit = j;
                break;
            }
        }
        data_buffer[i/2]=h_bit<<4|l_bit;

    }
}

uint16_t my_strstr( uint8_t *str_src, const char *str_dst, uint16_t src_len,uint16_t dst_len)
{
    uint16_t ret=0,j=0;
    
    for(int i=0;i<src_len;i++)
    {
        for( j=0;j<dst_len;j++)
        {
            if(str_src[i+j]!=str_dst[j])
            {
                break;
            }
        }
        if(j==dst_len)
        {
            ret = i+j;
            break;
        }
    }
    return ret;
}
#if 1
void comput_hmac_key(void)
{
    uint8_t local_salt[16];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *mdInfo = NULL;
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mdInfo, 1);
    memcpy(local_salt,sess_sn1,8 );
    memcpy(local_salt+8,sess_sn2,8 );
    
     mbedtls_pkcs5_pbkdf2_hmac(&ctx,sessionKey, 16, local_salt, 16, 1, 32, hmacKey); //hmacKey
    for(int i=0;i<32;i++)
    uart_printf("0x%x,",hmacKey[i]);
    uart_printf("\r\n");
}
void comput_session_key(void)
{
    uint8_t local_salt[16];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *mdInfo = NULL;
    mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_init(&ctx);
    
    mbedtls_md_setup(&ctx, mdInfo, 1);
    
    memcpy(local_salt,sess_sn1,8 );
    memcpy(local_salt+8,sess_sn2,8 );

    mbedtls_pkcs5_pbkdf2_hmac(&ctx, authCode, 16, local_salt, 16, 1, 16, sessionKey); 
    for(int i=0;i<16;i++)
    uart_printf("0x%x,",sessionKey[i]);

    uart_printf("\r\n");
   
}
#endif
uint16_t set_deviceinfo_cmdrsp(uint8_t *data_buffer)
{
    uint16_t data_len=0;
    char * temp_str;

    temp_str="{\"productId\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,prodId,strlen(prodId));
    data_len=data_len+strlen(prodId);

    temp_str ="\",\"sn\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,PRODUCT_SN,strlen(PRODUCT_SN));
    data_len=data_len+strlen(PRODUCT_SN);
    
    temp_str ="\",\"vendor\":{\"devId\":\"\",\"deviceInfo\":{\"model\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,deviceModel,strlen(deviceModel));
    data_len=data_len+strlen(deviceModel); 

    temp_str = "\",\"manu\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,manufacturerNameEn,strlen(manufacturerNameEn));
    data_len=data_len+strlen(manufacturerNameEn); 

    temp_str = "\",\"prodId\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,prodId,strlen(prodId));
    data_len=data_len+strlen(prodId); 

    temp_str = "\",\"mac\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,mac_str,sizeof(mac_str));
    data_len=data_len+sizeof(mac_str);

    temp_str = "\",\"blemac\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,mac_str,sizeof(mac_str));
    data_len=data_len+sizeof(mac_str);

    temp_str = "\",\"fwv\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,firmwareVer,strlen(firmwareVer));
    data_len=data_len+strlen(firmwareVer);

    temp_str = "\",\"hwv\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,hardwareVer,strlen(hardwareVer));
    data_len=data_len+strlen(hardwareVer);

    temp_str = "\",\"swv\":\"";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);
    memcpy(data_buffer+data_len,softwareVer,strlen(softwareVer));
    data_len=data_len+strlen(softwareVer);

    temp_str = "\",\"prot_t\":\"4\"}}}";
    memcpy(data_buffer+data_len,temp_str,strlen(temp_str));
    data_len=data_len+strlen(temp_str);

    
    return data_len;
}




/// Application Task Descriptor
extern const struct ke_task_desc TASK_DESC_APP;

/// List of functions used to create the database
static const appm_add_svc_func_t appm_add_svc_func_list[APPM_SVC_LIST_STOP] =
{
    #if (BLE_APP_FEE0S)
    (appm_add_svc_func_t)app_fee0_add_fee0s,
    #endif //(BLE_APP_FEE0S)
        
    #if (BLE_APP_HT)
    (appm_add_svc_func_t)app_ht_add_hts,
    #endif //(BLE_APP_HT)
    #if (BLE_APP_DIS)
    (appm_add_svc_func_t)app_dis_add_dis,
    #endif //(BLE_APP_DIS)
    #if (BLE_APP_BATT)
    (appm_add_svc_func_t)app_batt_add_bas,
    #endif //(BLE_APP_BATT)
    #if (BLE_APP_HID)
    (appm_add_svc_func_t)app_hid_add_hids,
    #endif //(BLE_APP_HID)
    #if (BLE_APP_AM0)
    (appm_add_svc_func_t)app_am0_add_has,
    #endif //(BLE_APP_AM0)
    #if (BLE_APP_ANCS)
    (appm_add_svc_func_t)app_ancs_add_ancsc,
    #endif
    #if (BLE_APP_OADS)
    (appm_add_svc_func_t)app_oad_add_oads,
    #endif

    #if (BLE_APP_FCC0S)
    (appm_add_svc_func_t)app_fcc0_add_fcc0s,
    #endif //(BLE_APP_FCC0S)

    #if (BLE_APP_OTAHS)
    (appm_add_svc_func_t)app_otah_add_otahs,
    #endif
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env;

uint8_t cur_adv_data_type=adv_data_type_unreg;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (!BLE_APP_AM0)
static void appm_build_adv_data(uint16_t max_length, uint16_t *p_length, uint8_t *p_buf)
{
    // Remaining Length
    //uint8_t rem_len = max_length;

    
    uart_printf("set adv data:%d \r\n",cur_adv_data_type);

    if(cur_adv_data_type == adv_data_type_first)
    {
        memcpy(p_buf, myadvData_first, sizeof(myadvData_first));
        *p_length += sizeof(myadvData_first);
        //p_buf += sizeof(myadvData_first);
    }else if(cur_adv_data_type == adv_data_type_sec)
    {
        memcpy(p_buf, myadvData_sec, sizeof(myadvData_sec));
        *p_length += sizeof(myadvData_sec);
        //p_buf += sizeof(myadvData_sec);
    }else if(cur_adv_data_type == adv_data_type_reg)
    {
        memcpy(p_buf, myadvData_reg, sizeof(myadvData_reg));
        *p_length += sizeof(myadvData_reg);
        //p_buf += sizeof(myadvData_reg);
    }else if(cur_adv_data_type == adv_data_type_unreg)
    {
        memcpy(p_buf, myadvData_unreg, sizeof(myadvData_unreg));
        *p_length += sizeof(myadvData_unreg);
        //p_buf += sizeof(myadvData_unreg);
    }
#if 0       
    #if 0
    *p_length = sizeof( remote_pair_adv_data );

    memcpy( p_buf, remote_pair_adv_data, *p_length);

    #else
    
    //adv type flag
    p_buf[0]=0x02;
    p_buf[1]=0x01;
    p_buf[2]=0x06;
    p_buf += 3;
    *p_length += 3;

 
    #if (BLE_APP_HT)
    // Set list of UUIDs
    memcpy(p_buf, APP_HT_ADV_DATA_UUID, APP_HT_ADV_DATA_UUID_LEN);
    *p_length += APP_HT_ADV_DATA_UUID_LEN;
    p_buf += APP_HT_ADV_DATA_UUID_LEN;

    // Set appearance
    memcpy(p_buf, APP_HT_ADV_DATA_APPEARANCE, APP_ADV_DATA_APPEARANCE_LEN);
    *p_length += APP_ADV_DATA_APPEARANCE_LEN;
    p_buf += APP_ADV_DATA_APPEARANCE_LEN;
    #endif //(BLE_APP_HT)

    #if (BLE_APP_HID)
    // Set list of UUIDs
    memcpy(p_buf, APP_HID_ADV_DATA_UUID, APP_HID_ADV_DATA_UUID_LEN);
    *p_length += APP_HID_ADV_DATA_UUID_LEN;
    p_buf += APP_HID_ADV_DATA_UUID_LEN;

    // Set appearance
    memcpy(p_buf, APP_HID_ADV_DATA_APPEARANCE, APP_ADV_DATA_APPEARANCE_LEN);
    *p_length += APP_ADV_DATA_APPEARANCE_LEN;
    p_buf += APP_ADV_DATA_APPEARANCE_LEN;
    #endif //(BLE_APP_HID)

    // Sanity check
    ASSERT_ERR(rem_len >= max_length);

    // Get remaining space in the Advertising Data - 2 bytes are used for name length/flag
    rem_len -= *p_length;

    // Check if additional data can be added to the Advertising data - 2 bytes needed for type and length
    if (rem_len > 2)
    {
        uint8_t dev_name_length = co_min(app_env.dev_name_len, (rem_len - 2));

        // Device name length
        *p_buf = dev_name_length + 1;
        // Device name flag (check if device name is complete or not)
        *(p_buf + 1) = (dev_name_length == app_env.dev_name_len) ? '\x09' : '\x08';
        // Copy device name
        memcpy(p_buf + 2, app_env.dev_name, dev_name_length);

        // Update advertising data length
        *p_length += (dev_name_length + 2);
    }
#endif

#endif    
}
#endif //(!BLE_APP_AM0)

void appm_start_advertising(void)
{
    // Prepare the GAPM_ACTIVITY_START_CMD message
    struct gapm_activity_start_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_START_CMD,
                                                         TASK_GAPM, TASK_APP,
                                                         gapm_activity_start_cmd);

    p_cmd->operation = GAPM_START_ACTIVITY;
    p_cmd->actv_idx = app_env.adv_actv_idx;
    #if (BLE_APP_HID)
    p_cmd->u_param.adv_add_param.duration = 0;
    #else //(BLE_APP_HID)
    p_cmd->u_param.adv_add_param.duration = 0;
    #endif //(BLE_APP_HID)
    p_cmd->u_param.adv_add_param.max_adv_evt = 0;

    // Send the message
    ke_msg_send(p_cmd);

    // Keep the current operation
    app_env.adv_state = APP_ADV_STATE_STARTING;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_START_ACTIVITY;
}


void appm_stop_advertising(void)
{
    // Prepare the GAPM_ACTIVITY_STOP_CMD message
    struct gapm_activity_stop_cmd *cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_STOP_CMD,
                                                      TASK_GAPM, TASK_APP,
                                                      gapm_activity_stop_cmd);

    // Fill the allocated kernel message
    cmd->operation = GAPM_STOP_ACTIVITY;
    cmd->actv_idx = app_env.adv_actv_idx;

    // Send the message
    ke_msg_send(cmd);

    // Update advertising state
    app_env.adv_state = APP_ADV_STATE_STOPPING;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_STOP_ACTIVITY;
}


void appm_set_adv_data(void)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_ADV_DATA;
    p_cmd->actv_idx = app_env.adv_actv_idx;

    p_cmd->length = 0;
    // GAP will use 3 bytes for the AD Type
    appm_build_adv_data(ADV_DATA_LEN, &p_cmd->length, &p_cmd->data[0]);
   
    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    app_env.adv_state = APP_ADV_STATE_SETTING_ADV_DATA;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_SET_ADV_DATA;
}

void appm_set_scan_rsp_data(void)
{
    // Prepare the GAPM_SET_ADV_DATA_CMD message
    struct gapm_set_adv_data_cmd *p_cmd = KE_MSG_ALLOC_DYN(GAPM_SET_ADV_DATA_CMD,
                                                           TASK_GAPM, TASK_APP,
                                                           gapm_set_adv_data_cmd,
                                                           ADV_DATA_LEN);

    // Fill the allocated kernel message
    p_cmd->operation = GAPM_SET_SCAN_RSP_DATA;
    p_cmd->actv_idx = app_env.adv_actv_idx;
    uart_printf("set scan rsp data:%d \r\n",cur_adv_data_type);
    if(cur_adv_data_type == adv_data_type_first)
    {
        p_cmd->length = sizeof(myrspData_first);
        memcpy(&p_cmd->data[0], myrspData_first, sizeof(myrspData_first));
    }else if(cur_adv_data_type == adv_data_type_sec)
    {
        
        p_cmd->length = sizeof(myrspData_sec);
        memcpy(&p_cmd->data[0], myrspData_sec, sizeof(myrspData_sec));
        
    }else if(cur_adv_data_type == adv_data_type_reg)
    {
        p_cmd->length = sizeof(myrspData_reg);
        memcpy(&p_cmd->data[0], myrspData_reg, sizeof(myrspData_reg));
    }else if(cur_adv_data_type == adv_data_type_unreg)
    {
        p_cmd->length = sizeof(myrspData_unreg);
        memcpy(&p_cmd->data[0], myrspData_unreg, sizeof(myrspData_unreg));
    }
    
    // Send the message
    ke_msg_send(p_cmd);

    // Update advertising state
    app_env.adv_state = APP_ADV_STATE_SETTING_SCAN_RSP_DATA;
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_SET_SCAN_RSP_DATA;
}


static void appm_send_gapm_reset_cmd(void)
{
    // Reset the stack
    struct gapm_reset_cmd *p_cmd = KE_MSG_ALLOC(GAPM_RESET_CMD,
                                                TASK_GAPM, TASK_APP,
                                                gapm_reset_cmd);

    p_cmd->operation = GAPM_RESET;

    ke_msg_send(p_cmd);
}

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void appm_init()
{
    // Reset the application manager environment
    memset(&app_env, 0, sizeof(app_env));

    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APPM_INIT);

    flash_read_data(&app_env.dev_name[0],(flash_env.bdaddr_def_addr_abs+8),APP_DEVICE_NAME_MAX_LEN);

    for(uint8_t i=0;i<APP_DEVICE_NAME_MAX_LEN;i++)
    {
        if( (app_env.dev_name[i]!=0xff) && (app_env.dev_name[i]!=0x00) )
        {
            app_env.dev_name_len++;
        }
        else
        {
            break;
        }

    }       
    if( (app_env.dev_name[0]==0xff) && (app_env.dev_name[1]==0xff) )
    {
        // Get default Device Name (No name if not enough space)
        memcpy(app_env.dev_name, APP_DFLT_DEVICE_NAME, APP_DFLT_DEVICE_NAME_LEN);
        app_env.dev_name_len = APP_DFLT_DEVICE_NAME_LEN;

        // TODO update this value per profiles
    }


    /*------------------------------------------------------
     * INITIALIZE ALL MODULES
     *------------------------------------------------------*/

    // load device information:

    #if (DISPLAY_SUPPORT)
    app_display_init();
    #endif //(DISPLAY_SUPPORT)

    #if (BLE_APP_SEC)
    // Security Module
    app_sec_init();
    #endif // (BLE_APP_SEC)

    #if (BLE_APP_HT)
    // Health Thermometer Module
    app_ht_init();
    #endif //(BLE_APP_HT)

    #if (BLE_APP_DIS)
    // Device Information Module
    app_dis_init();
    #endif //(BLE_APP_DIS)

    #if (BLE_APP_HID)
    // HID Module
    app_hid_init();
    #endif //(BLE_APP_HID)

    #if (BLE_APP_BATT)
    // Battery Module
    app_batt_init();
    #endif //(BLE_APP_BATT)

    #if (BLE_APP_AM0)
    // Audio Mode 0 Module
    app_am0_init();
    #endif //(BLE_APP_AM0)
    
    #if (BLE_APP_ANCS)
    app_ancsc_init();
    #endif
    #if (BLE_APP_OADS)
    app_oads_init();        
    #endif
    
    #if (BLE_APP_OTAHS)
    app_otah_init();   
    #endif
    
    mac_hex_to_str((uint8_t *)mac_str,co_default_bdaddr);
    uart_printf("mac_str: ");
    for(uint8_t i=0;i<sizeof(mac_str);i++)
    {
        uart_printf("%c",mac_str[i]);

    }
    uart_printf("\r\n");
    if(0)
    {
        uint16_t buf_len;
        uint8_t buf[500];
        //extern uint8_t sess_id[32];
        buf_len = set_deviceinfo_cmdrsp(buf);
        uart_printf("buf_len=%d\r\n",buf_len);
        for(int i=0;i<buf_len;i++)
        {
            uart_printf("%c",buf[i]);
            if(buf[i]==',')
                uart_printf("\r\n");
        }
        

    }
                    uart_printf("sesskey:\r\n");

   // comput_session_key();
                    uart_printf("\r\n");
   // comput_hmac_key();
    // Reset the stack
    appm_send_gapm_reset_cmd();
}

bool appm_add_svc(void)
{
    // Indicate if more services need to be added in the database
    bool more_svc = false;

    // Check if another should be added in the database
    if (app_env.next_svc != APPM_SVC_LIST_STOP)
    {
        ASSERT_INFO(appm_add_svc_func_list[app_env.next_svc] != NULL, app_env.next_svc, 1);

        // Call the function used to add the required service
        appm_add_svc_func_list[app_env.next_svc]();

        // Select following service to add
        app_env.next_svc++;
        more_svc = true;
    }
    uart_printf("%s more_svc:%d\r\n",__func__,more_svc);
    return more_svc;
}

void appm_disconnect(void)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                   gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason    = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}
void rw_ip_sleep_test(uint32_t time);
void rw_ip_deep_sleep_test(void);
void appm_create_advertising(void)
{

    if (app_env.adv_state == APP_ADV_STATE_IDLE)
    {
        // Prepare the GAPM_ACTIVITY_CREATE_CMD message
        struct gapm_activity_create_adv_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_CREATE_CMD,
                                                                  TASK_GAPM, TASK_APP,
                                                                  gapm_activity_create_adv_cmd);

        // Set operation code
        p_cmd->operation = GAPM_CREATE_ADV_ACTIVITY;


        // Fill the allocated kernel message
        p_cmd->own_addr_type = GAPM_STATIC_ADDR;
        p_cmd->adv_param.type = GAPM_ADV_TYPE_LEGACY;
        p_cmd->adv_param.prop = GAPM_ADV_PROP_UNDIR_CONN_MASK;
        p_cmd->adv_param.filter_pol = ADV_ALLOW_SCAN_ANY_CON_ANY;
        p_cmd->adv_param.prim_cfg.chnl_map = APP_ADV_CHMAP;
        p_cmd->adv_param.prim_cfg.phy = GAP_PHY_LE_1MBPS;
        p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_GEN_DISC;
        
  

        /*
         * If the peripheral is already bonded with a central device, use the direct advertising
         * procedure (BD Address of the peer device is stored in NVDS.
         */
        if (0)//app_sec_get_bond_status())
        {
            // BD Address of the peer device
           struct gap_bdaddr peer_bd_addr;
//            // Length
           uint8_t length = NVDS_LEN_PEER_BD_ADDRESS;
            uart_printf("---appm_create_advertising derect\r\n");
//            // Get the BD Address of the peer device in NVDS
           if (nvds_get(NVDS_TAG_PEER_BD_ADDRESS, &length, (uint8_t *)&peer_bd_addr) != NVDS_OK)
           {
               // The address of the bonded peer device should be present in the database
               ASSERT_ERR(0);
            }

            //p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
           // Set the DIRECT ADVERTISING mode
           // p_cmd->adv_param.prop = GAPM_ADV_PROP_DIR_CONN_MASK;
            // Copy the BD address of the peer device and the type of address
            memcpy(&p_cmd->adv_param.peer_addr, &peer_bd_addr, NVDS_LEN_PEER_BD_ADDRESS);
            
            p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_FAST_INT;
            p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_FAST_INT;
            uart_printf("peeraddr1=%x,%x,%x,%x,%x,%x\r\n",p_cmd->adv_param.peer_addr.addr.addr[0],p_cmd->adv_param.peer_addr.addr.addr[1],p_cmd->adv_param.peer_addr.addr.addr[2],
                                    p_cmd->adv_param.peer_addr.addr.addr[3],p_cmd->adv_param.peer_addr.addr.addr[4],p_cmd->adv_param.peer_addr.addr.addr[5]);  
            uart_printf("peeraddr2=%x,%x,%x,%x,%x,%x\r\n",peer_bd_addr.addr.addr[0],peer_bd_addr.addr.addr[1],peer_bd_addr.addr.addr[2],
                                    peer_bd_addr.addr.addr[3],peer_bd_addr.addr.addr[4],peer_bd_addr.addr.addr[5]);  
            app_env.adv_prop = APP_ADV_PROP_DERECT;
            p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_NON_DISC;
        }
        else
        {
            uart_printf("appm_create_advertising nonderect \r\n");
            //p_cmd->adv_param.disc_mode = GAPM_ADV_MODE_LIM_DISC;
            p_cmd->adv_param.prim_cfg.adv_intv_min = APP_ADV_INT_MIN;
            p_cmd->adv_param.prim_cfg.adv_intv_max = APP_ADV_INT_MAX;
            app_env.adv_prop = APP_ADV_PROP_NONDERECT;
            
        }

      

        // Send the message
        ke_msg_send(p_cmd);

        // Keep the current operation
        app_env.adv_state = APP_ADV_STATE_CREATING;
        // And the next expected operation code for the command completed event
        app_env.adv_op = GAPM_CREATE_ADV_ACTIVITY;
    }
}


void appm_delete_advertising(void)
{
    // Prepare the GAPM_ACTIVITY_CREATE_CMD message
    struct gapm_activity_delete_cmd *p_cmd = KE_MSG_ALLOC(GAPM_ACTIVITY_DELETE_CMD,
                                                              TASK_GAPM, TASK_APP,
                                                              gapm_activity_delete_cmd);

    // Set operation code
    p_cmd->operation = GAPM_DELETE_ALL_ACTIVITIES;

    // Send the message
    ke_msg_send(p_cmd);
    uart_printf("appm_delete_advertising \r\n");
    // Keep the current operation
    // And the next expected operation code for the command completed event
    app_env.adv_op = GAPM_DELETE_ALL_ACTIVITIES;
}


void appm_adv_fsm_next(void)
{
    uart_printf("%s adv_state:%x\r\n",__func__,app_env.adv_state);
    switch (app_env.adv_state)
    {
        case (APP_ADV_STATE_IDLE):
        {
            // Create advertising
            appm_create_advertising();
        } break;

        case (APP_ADV_STATE_CREATING):
        {
            // Set advertising data
            appm_set_adv_data();
        } break;

        case (APP_ADV_STATE_SETTING_ADV_DATA):
        {
            // Set scan response data
            appm_set_scan_rsp_data();
        } break;

        case (APP_ADV_STATE_CREATED):
        case (APP_ADV_STATE_SETTING_SCAN_RSP_DATA):
        {
            // Start advertising activity
            appm_start_advertising();
        } break;

        case (APP_ADV_STATE_STARTING):
        {
            #if (DISPLAY_SUPPORT)
            // Update advertising state screen
            app_display_set_adv(true);
            #endif //(DISPLAY_SUPPORT)

            // Go to started state
            app_env.adv_state = APP_ADV_STATE_STARTED;
        } break;

        case (APP_ADV_STATE_STARTED):
        {
            // Stop advertising activity
            appm_stop_advertising();
        } break;

        case (APP_ADV_STATE_STOPPING):
        {
            #if (DISPLAY_SUPPORT)
            // Update advertising state screen
            app_display_set_adv(false);
            #endif //(DISPLAY_SUPPORT)

            // Go created state
            app_env.adv_state = APP_ADV_STATE_CREATED;
        } break;



        default:
        {
            ASSERT_ERR(0);
        } break;
    }
}


void appm_update_param(struct gapc_conn_param *conn_param)
{
    // Prepare the GAPC_PARAM_UPDATE_CMD message
    struct gapc_param_update_cmd *cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                                     KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                                     gapc_param_update_cmd);

    cmd->operation  = GAPC_UPDATE_PARAMS;
    cmd->intv_min   = conn_param->intv_min;
    cmd->intv_max   = conn_param->intv_max;
    cmd->latency    = conn_param->latency;
    cmd->time_out   = conn_param->time_out;

    // not used by a slave device
    cmd->ce_len_min = 0xFFFF;
    cmd->ce_len_max = 0xFFFF;

    // Send the message
    ke_msg_send(cmd);
}

uint8_t appm_get_dev_name(uint8_t* name)
{
    // copy name to provided pointer
    memcpy(name, app_env.dev_name, app_env.dev_name_len);
    // return name length
    return app_env.dev_name_len;
}

void appm_update_adv_state(bool start)
{
    // TODO [LT] - Check current advertising state
    uart_printf("appm_update_adv_state \r\n");
    if(app_sec_get_bond_status())
    {
        if(app_env.adv_prop==APP_ADV_PROP_NONDERECT)
        {
        
            appm_delete_advertising();
        }else
        {
            appm_adv_fsm_next();
        }
    }else
    {
        if(app_env.adv_prop==APP_ADV_PROP_DERECT)
        {
        
            appm_delete_advertising();
        }else
        {
            appm_adv_fsm_next();
        }

    }
    
    
}


void latency_disable_set(uint8_t value)
{
    extern uint8_t latency_disable;
    latency_disable=value;
}


#endif //(BLE_APP_PRESENT)

/// @} APP
