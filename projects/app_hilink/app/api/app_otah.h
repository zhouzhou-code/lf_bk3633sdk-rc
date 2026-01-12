/**
 ****************************************************************************************
 *
 * @file app_otah.h
 *
 * @brief fee0 Application Module entry point
 *
 * @auth  gang.cheng
 *
 * @date  2020.03.17
 *
 * Copyright (C) Beken 2020-2022
 *
 *
 *
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief otah Application Module entry point
 *
 * @{
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration


#include <stdint.h>          // Standard Integer Definition
#include "ke_task.h"         // Kernel Task Definition

/*
 * STRUCTURES DEFINITION
 ****************************************************************************************
 */

/// fee0s Application Module Environment Structure
struct app_otah_env_tag
{
    /// Connection handle
    uint8_t conidx;
    
};
/*
 * GLOBAL VARIABLES DECLARATIONS
 ****************************************************************************************
 */
typedef struct
{
    uint32_t update_offset;
    uint32_t data_cnt;
    uint32_t  otah_part;// app bin  or  app&stack bin
    uint8_t  data[2000];
} OTAH_SECTION_T;
/// fee0s Application environment
extern struct app_otah_env_tag app_otah_env;

/// Table of message handlers
extern const struct app_subtask_handlers app_otah_handler;

/*
 * FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 *
 * otahs Application Functions
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize fff0s Application Module
 ****************************************************************************************
 */
void app_otah_init(void);
/**
 ****************************************************************************************
 * @brief Add a fee0 Service instance in the DB
 ****************************************************************************************
 */
void app_otah_add_otahs(void);


/**
 ****************************************************************************************
 * @brief Send a ctrl  value
 ****************************************************************************************
 */
void app_ctrl_send_ind(uint8_t conidx,uint16_t len,uint8_t* buf);

