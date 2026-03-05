#ifndef OLED_CONFIG_H
#define OLED_CONFIG_H

#include "pinmap_v1.h"

// Screen configuration
#define USE_HORIZONTAL 0
#define LCD_W 120
#define LCD_H 240

// DMA / batching configuration
#define DMA_SIZE 930
#define USE_DMA 0
#define SPI_BATCH_N 64
#define SPI_PIC_BATCH_N 64

// GPIO mapping (edit as needed for your board)
#define LCD_SCK_GPIO_PIN  LCD_SPI_SCK
#define LCD_MOSI_GPIO_PIN LCD_SPI_MOSI
#define LCD_CS_GPIO_PIN   LCD_SPI_CS
#define LCD_DC_GPIO_PIN   LCD_DCX
#define LCD_RES_GPIO_PIN  LCD_RST
// Optional extra control pin (reserved by default)
#define LCD_KEY1_GPIO_PIN Port_Pin(2, 5)

#endif // OLED_CONFIG_H
