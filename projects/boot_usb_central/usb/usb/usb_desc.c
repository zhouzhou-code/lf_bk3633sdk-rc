/*************************************************************
 * @file        usb_desc.c
 * @brief        USB descriptors ,discribed USB spec1.1 chapter 9
 * @author        Jiang Kaigan
 * @version        V1.0
 * @date        2020-12-03
 * @par
 * @attention
 *
 * @history        2020-12-03 jkg    create this file
 */

/*file type declaration section*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*internal symbols import section*/
#include "usbclass.h"
#include "usb.h"
#include "usbdef.h"
#include "..\driver_usb.h"

#pragma pack(1)
typedef struct {
    int ind;
    CBufferBaseDesc desc;
} _t_index_descriptor;
#pragma pack()


const UI8 tHidDbgReportDesc[]={
    HidUsagePage(HID_USAGEPAGE_Customer),
    HidUsage(0x01),
    HidCollectionApp(),
    HidUsage(0x00),
    HidReportBits(8),
    HidReportCount(64),
    HidLogiMin8(0),
    HidLogiMax16(0xff),
    HidInput(0, 1, 0),
    HidUsage(0x00),
    HidReportBits(8),
    HidReportCount(64),
    HidLogiMin8(0),
    HidLogiMax16(0xff),
    HidOutput(0, 1, 0),
    HidEndCollection(),
};
const UI16 tUsbLanguageID[]={
    0x04+(USB_DESCTYPE_STRING<<8),
    _LANGUAGE_ID_
};

const UI16 tUsbProductStr[]={
    (((9+1)*2))+(USB_DESCTYPE_STRING<<8),'H','I','D',' ','M','o','u','s','e',
};

const UI16 tUsbVendorStr[]={
    (((5+1)*2))+(USB_DESCTYPE_STRING<<8),'B','e','k','e','n'
};

const CUsbDevDesc tUsbDeviceDescriptor={
    sizeof(CUsbDevDesc),
    USB_DESCTYPE_DEVICE,
    USB_VERSION,
    USB_DEVICE_CLASS(0),
    USB_DEVICE_SUBCLASS(0),
    USB_DEVICE_PROTOCOL(0),
    USB_MAX_PACKET_LEN(64),
    _VID_,
    _PID_,
    0x0101,
    _Vendor_String_ID_,
    _Product_String_ID_,
    0,
    USB_DEVICE_CONFIG_NUM(USB_MAX_CONFIG_NUM)
};

#if 1

#define SZ_HID_Intf(nEndp)                9+/*HID Interface descriptor*/+\
    9    /*HID class descriptor*/+\
    7*(nEndp)    /*Endpoint descriptor*/+\
    0

#define SZ_CONFIG(nMicSR,nSpkSR,nIntfAs)        9/*config*/+\
    SZ_HID_Intf(2)/*HID DBG descs size*/+\
    0

const UI8 tUsbConfig[]={
    USB_Gen_Config(SZ_CONFIG(0,0,0), USB_MAX_INTERFACE_NUM, 100),

    USB_INTERFACE_DESC(USB_INTFID_Hid_Dbg, 0, 2, USB_DEVICE_CLASS_HID, 0,0),
    USB_HID_CLS_DESC(tHidDbgReportDesc),
    USB_HID_EndpIN(USB_ENDPID_Hid_DBG_IN, 64, 1),
    USB_HID_EndpOUT(USB_ENDPID_Hid_DBG_OUT, 64, 1),

};
#endif

const CBufferBaseDesc _c_device_desc={(void*)&tUsbDeviceDescriptor,sizeof(tUsbDeviceDescriptor)};
const CBufferBaseDesc _c_config_desc={(void*)tUsbConfig,sizeof(tUsbConfig)};
const _t_index_descriptor tblUsbHidRptDescs[]={
    {USB_INTFID_Hid_Dbg,{(void*)tHidDbgReportDesc,sizeof(tHidDbgReportDesc)}},
};
const _t_index_descriptor tblUsbStringDescs[]={
    {0,{(void*)tUsbLanguageID,sizeof(tUsbLanguageID)}},
    {_Vendor_String_ID_,{(void*)tUsbVendorStr,sizeof(tUsbVendorStr)}},
    {_Product_String_ID_,{(void*)tUsbProductStr,sizeof(tUsbProductStr)}},
};

void*USBDesc_GetDeviceDesc(void){
    return (void*)&_c_device_desc;
}

void*USBDesc_GetConfigDesc(void){
    return (void*)&_c_config_desc;
}
void*UsbDesc_Find(int idx,void*tb,int cnt){
    _t_index_descriptor*ptbl=(_t_index_descriptor*)tb;
    int i;
    for(i=0;i<cnt;i++){
        if(ptbl[i].ind==idx){
            return (void*)&ptbl[i].desc;
        }
    }
    return(NULL);
}

void*USBDesc_GetHidRptDesc(int idx){
    return UsbDesc_Find(idx,(void*)tblUsbHidRptDescs,GET_ELEMENT_TBL(tblUsbHidRptDescs));
}

void*USBDesc_GetStringDesc(int idx){
    return UsbDesc_Find(idx,(void*)tblUsbStringDescs,GET_ELEMENT_TBL(tblUsbStringDescs));
}

