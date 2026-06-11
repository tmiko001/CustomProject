#ifndef COMMON_H
#define COMMON_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

// --- SPI Defines ---
#define SPI_PORT spi0
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_DATA_CMD 20  
#define PIN_CS_LED   21  

// --- Display Commands ---
#define CMD_DISPLAYON 0xAF
#define CMD_DISPLAYOFF 0xAE
#define CMD_SETCOLUMNADDRESS 0x15
#define CMD_SETROWADDRESS 0x75
#define CMD_CLEARWINDOW 0x25
#define CMD_FILLWINDOW 0x26

// --- RGB565 Colors (16-bit) ---
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F
#define COLOR_WHITE 0xFFFF
#define COLOR_YELLOW 0xFFE0
#define COLOR_BLACK 0x0000

// --- Shared Quadrant Enum ---
typedef enum { 
    QUAD_NONE = -1, 
    QUAD_TL = 0, 
    QUAD_TR = 1, 
    QUAD_BL = 2, 
    QUAD_BR = 3 
} Quadrant_t;

#endif // COMMON_H