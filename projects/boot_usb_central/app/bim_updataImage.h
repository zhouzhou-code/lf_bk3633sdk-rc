#include <stdint.h>
#define BOOT_MAX_SIZE                        (0x2000)
#define FLASH_SEC_SIZE                       (0X1000)
#define FLASH_ONE_BLOCK_SIZE                 (0X10000)//64k
#define SEC_MAX_FSIZE_APP_BLOCK              (0x2A00)
#define SEC_MAX_FSIZE_STACK_BLOCK            (0x3E00)

#define SEC_IMAGE_RUN_STACK_CADDR            (0x4c00 )///==(BOOT_MAX_SIZE/272+1)*256
#define SEC_IMAGE_RUN_STACK_FADDR            (SEC_IMAGE_RUN_STACK_CADDR*34/32) /////==(BOOT_MAX_SIZE/272+1)*272
#define SEC_IMAGE_OAD_HEADER_STACK_FADDR     (SEC_IMAGE_RUN_STACK_FADDR-0x10) /////==(BOOT_MAX_SIZE/272+1)*272-0x10
#define SEC_IMAGE_RUN_APP_CADDR              (0x29700 )///==(BOOT_MAX_SIZE/272+1)*256
#define SEC_IMAGE_RUN_APP_FADDR              (SEC_IMAGE_RUN_APP_CADDR*34/32)/////==(BOOT_MAX_SIZE/272+1)*272
#define SEC_IMAGE_OAD_HEADER_APP_FADDR       (SEC_IMAGE_RUN_APP_FADDR-0x10)


#define SEC_ALL_IMAGE_ALLOC_START_8M_FADDR          (0x2000)
#define SEC_ALL_IMAGE_ALLOC_END_8M_FADDR            (0x80000) //(512KB)
#define SEC_ALL_BACKUP_OAD_HEADER_8M_FADDR          (0x80000) //512b * 1024
#define SEC_ALL_BACKUP_OAD_IMAGE_8M_FADDR           (0x80010) //512kb * 1024 + 0X10
#define SEC_ALL_BACKUP_OAD_END_8M_FADDR             (0xFC000) //(1024-16) * 1024

#define SEC_PART_IMAGE_ALLOC_START_8M_FADDR         (0x28000)
#define SEC_PART_IMAGE_ALLOC_END_8M_FADDR           (0x80000) //(512KB)
#define SEC_PART_BACKUP_OAD_HEADER_8M_FADDR         (0x80000) 
#define SEC_PART_BACKUP_OAD_IMAGE_8M_FADDR          (0x80010) //512KB * 1024 + 0X10
#define SEC_PART_BACKUP_OAD_END_8M_FADDR            (0xFC000) //(1024-16) * 1024

#define SEC_ALL_IMAGE_ALLOC_START_4M_FADDR          (0x2000)
#define SEC_ALL_IMAGE_ALLOC_END_4M_FADDR            (0x40000) //(256KB)
#define SEC_ALL_BACKUP_OAD_HEADER_4M_FADDR          (0x40000) //256kb * 1024
#define SEC_ALL_BACKUP_OAD_IMAGE_4M_FADDR           (0x40010) //256kb * 1024 + 0X10
#define SEC_ALL_BACKUP_OAD_END_4M_FADDR             (0x7C000) //(512-16) * 1024

#define SEC_PART_IMAGE_ALLOC_START_4M_FADDR         (0x28000)
#define SEC_PART_IMAGE_ALLOC_END_4M_FADDR           (0x52000) 
#define SEC_PART_BACKUP_OAD_HEADER_4M_FADDR         (0x52000) 
#define SEC_PART_BACKUP_OAD_IMAGE_4M_FADDR          (0x52010) 
#define SEC_PART_BACKUP_OAD_END_4M_FADDR            (0x7C000) //(512-16) * 1024


#define OAD_APP_PART_UID                    (0x42424242)
#define OAD_APP_STACK_UID                   (0x53535353)

#define ERASE_FLASH_SEC_SIZE                 0X20   // 4K
#define ERASE_FLASH_ONE_BLOCK_SIZE           0XD8   //64K  erase cmd
/*********************************************************************
 * TYPEDEFS
 */
// The Image Header will not be encrypted, but it will be included in a Signature.
typedef struct
{
    // Secure OAD uses the Signature for image validation instead of calculating a CRC, but the use
    // of CRC==CRC-Shadow for quick boot-up determination of a validated image is still used.
    uint32_t crc;       // CRC must not be 0x0000 or 0xFFFF.
    // User-defined Image Version Number - default logic uses simple a '!=' comparison to start an OAD.
    uint16_t ver;

    uint16_t len;        // Image length in 4-byte blocks (i.e. HAL_FLASH_WORD_SIZE blocks).

    uint32_t  uid;       // User-defined Image Identification bytes.
    uint8_t  crc_status;     // cur image crc status
    uint8_t  sec_status;     // cur image sec status
    uint16_t  rom_ver;     // Rom ver.
} img_hdr_t;
#define BLOCK_SIZE          0X10
#define READ_BLOCK_SIZE     0x100
#define CRC_UNCHECK         0xFF
#define CRC_CHECK_OK        0xAA
#define CRC_CHECK_FAIL      0x55
#define SECT_UNKNOW         0xFF
#define SECT_NORMAL         0xAA
#define SECT_ABNORMAL       0x55
enum
{
    SSTATUS_SECT_NORMAL = 0,
    SSTATUS_SECT_ERASED,
    SSTATUS_SECT_ABNORMAL,
    SSTATUS_SECT_UNKOWN ,
    SSTATUS_SECT_DIFF_ROM_VER ,
};
void bim_erase_image_sec(void);
void bim_erase_backup_sec(void);
void bim_updata_backup_to_image_sec(void);
void bim_updata_image_to_backup_sec(void);
void bim_test_crc(void);
void bim_test_erase_time(void);
uint8_t bim_select_sec(void);
int make_crc32_table(void);
uint32_t make_crc32(uint32_t crc,unsigned char *string,uint32_t size);
uint32_t bim_image_header_check(void);
