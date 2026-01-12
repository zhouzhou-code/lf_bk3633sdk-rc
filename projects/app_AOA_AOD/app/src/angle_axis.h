/**
 ****************************************************************************************
 *
 * @file angle_axis.h
 *
 * @brief Angle entry point
 *
 * Copyright (C) Beken 2020-2026
 *
 *
 ****************************************************************************************
 */
 
#ifndef ANGLE_AXIS_H_
#define ANGLE_AXIS_H_

#include <stdint.h>          // Standard Integer Definition
 /*
 * DEFINES
 ****************************************************************************************
 */
#define GPIOB_0     0x10
#define GPIOB_1     0x11
#define GPIOB_2     0x12
#define GPIOB_3     0x13
#define GPIOB_4     0x14
#define GPIOB_5     0x15
#define GPIOB_6     0x16
#define GPIOB_7     0x17

#define DEG_0  GPIOB_0
#define DEG_30  GPIOB_1
#define DEG_60  GPIOB_2
#define DEG_90  GPIOB_3
#define DEG_120  GPIOB_4
#define DEG_150  GPIOB_5
#define DEG_180  GPIOB_6

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
 
 /*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

typedef struct axis_t
{
    float x;
    float y;
}axis_t;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

float clac_iq2angle(float i[],float q[],uint16_t len);
struct axis_t clac_angle2axis(uint8_t num,float angle[],struct axis_t axis[]);



void deg_gpio_init(void);
void deg_gpio_set(float deg);

#endif // ANGLE_AXIS_H_

