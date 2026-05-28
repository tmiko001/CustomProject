//#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "upArrow_rgb565.h"
#include "image2_rgb565.h"
#include "image3_rgb565.h"
#include "image4_rgb565.h"
#include "downArrow_rgb565.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// GP19 = MOSI (pico), GP18 = SCK (sclk), GP17 = CS (cs_n)
#define SPI_PORT spi0
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_DATA_CMD 20  // Data/Command pin
#define PIN_CS_LED   21  // Extra chip select for WS2812 LED strip SPI

// Display Commands
#define CMD_DISPLAYON 0xAF
#define CMD_DISPLAYOFF 0xAE
#define CMD_SETCOLUMNADDRESS 0x15
#define CMD_SETROWADDRESS 0x75
#define CMD_CLEARWINDOW 0x25
#define CMD_FILLWINDOW 0x26

#define CMD_WS2812_WRITE_FRAME 0xA0
#define CMD_WS2812_SHOW 0xA1

// RGB565 Colors (16-bit)
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F
#define COLOR_WHITE 0xFFFF
#define COLOR_YELLOW 0xFFE0
#define COLOR_BLACK 0x0000

// Frame buffers for WS2812 LED strip data
static uint8_t led_strip0[60][3];
static uint8_t led_strip1[60][3];
static uint8_t led_strip2[60][3];
static uint8_t led_strip3[60][3];

// Helper function to send a byte via SPI
static void spi_write_byte(uint8_t byte) {
    spi_write_blocking(SPI_PORT, &byte, 1);
}

// Helper function to send a command (CS low, data_cmd low)
static void send_command(uint8_t cmd, const uint8_t *params, int param_count) {
    gpio_put(PIN_DATA_CMD, 0);
    gpio_put(PIN_CS, 0);

    spi_write_byte(cmd);
    for (int i = 0; i < param_count; i++) {
        spi_write_byte(params[i]);
    }

    gpio_put(PIN_CS, 1);
}

// Send a pixel byte in data mode
static void send_pixel_byte(uint8_t byte) {
    gpio_put(PIN_CS, 0);
    spi_write_byte(byte);
    gpio_put(PIN_CS, 1);
}

// Send a block of pixel bytes in data mode with CS held low
static void send_pixel_block(const uint8_t *data, int count) {
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, data, count);
    gpio_put(PIN_CS, 1);
}

// Send one full WS2812 strip frame (60 pixels, each GRB)
static void ws_send_frame(uint8_t bank, uint8_t pixels[60][3]) {
    gpio_put(PIN_CS_LED, 0);
    spi_write_byte(CMD_WS2812_WRITE_FRAME);
    spi_write_byte(bank & 0x03);
    spi_write_blocking(SPI_PORT, &pixels[0][0], 60 * 3);
    spi_write_byte(CMD_WS2812_SHOW);
    spi_write_byte(bank & 0x03);
    gpio_put(PIN_CS_LED, 1);
}

// Set a single WS2812 pixel in the array (GRB order)
static void ws_set_pixel(uint8_t pixels[60][3], int index, uint8_t g, uint8_t r, uint8_t b) {
    if (index < 0 || index >= 60) return;
    pixels[index][0] = g;
    pixels[index][1] = r;
    pixels[index][2] = b;
}

// Fill a window with a color
static void fill_window(uint16_t col_start, uint16_t col_end, uint16_t row_start, uint16_t row_end, uint16_t color) {
    uint8_t col_params[] = {(col_start >> 8) & 0xFF, col_start & 0xFF, (col_end >> 8) & 0xFF, col_end & 0xFF};
    uint8_t row_params[] = {(row_start >> 8) & 0xFF, row_start & 0xFF, (row_end >> 8) & 0xFF, row_end & 0xFF};
    
    send_command(CMD_SETCOLUMNADDRESS, col_params, 4);
    send_command(CMD_SETROWADDRESS, row_params, 4);
    sleep_ms(5);

    gpio_put(PIN_DATA_CMD, 1);
    sleep_ms(5);

    uint8_t color_high = (color >> 8) & 0xFF;
    uint8_t color_low = color & 0xFF;

    int width = (col_end - col_start + 1);
    int height = (row_end - row_start + 1);

    for (int i = 0; i < width * height; i++) {
        send_pixel_byte(color_high);
        send_pixel_byte(color_low);
    }

    gpio_put(PIN_DATA_CMD, 0);
    sleep_ms(5);
}

// Draw an RGB565 image at a pixel position (x,y)
static void draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels) {
    uint16_t col_start = x * 2;
    uint16_t col_end = col_start + width * 2 - 1;
    uint8_t col_params[] = {(col_start >> 8) & 0xFF, col_start & 0xFF, (col_end >> 8) & 0xFF, col_end & 0xFF};
    uint8_t row_params[] = {(y >> 8) & 0xFF, y & 0xFF, ((y + height - 1) >> 8) & 0xFF, (y + height - 1) & 0xFF};

    send_command(CMD_SETCOLUMNADDRESS, col_params, 4);
    send_command(CMD_SETROWADDRESS, row_params, 4);
    sleep_ms(5);

    gpio_put(PIN_DATA_CMD, 1);
    sleep_ms(5);

    for (int i = 0; i < width * height; i++) {
        uint16_t pixel = pixels[i];
        uint8_t pixel_bytes[2] = {(pixel >> 8) & 0xFF, pixel & 0xFF};
        send_pixel_block(pixel_bytes, 2);
    }

    gpio_put(PIN_DATA_CMD, 0);
    sleep_ms(5);
}

// Reset screen to black
static void screen_reset() {
    fill_window(0, 959, 0, 271, COLOR_BLACK);
}

// Set a quadrant (0-3) to a specific color
// 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
static void set_quadrant(uint16_t color, int quadrant) {
    uint16_t col_start, col_end, row_start, row_end;

    if (quadrant == 0) {
        col_start = 0;
        col_end = 479;
        row_start = 0;
        row_end = 135;
    } else if (quadrant == 1) {
        col_start = 480;
        col_end = 959;
        row_start = 0;
        row_end = 135;
    } else if (quadrant == 2) {
        col_start = 0;
        col_end = 479;
        row_start = 136;
        row_end = 271;
    } else { // quadrant == 3
        col_start = 480;
        col_end = 959;
        row_start = 136;
        row_end = 271;
    }

    fill_window(col_start, col_end, row_start, row_end, color);
}


int main()
{
    stdio_init_all();

    // SPI initialisation at 10MHz - 1MHz is stable, 10 is the max
    spi_init(SPI_PORT, 1000 * 5000);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_DATA_CMD, GPIO_FUNC_SIO);
    gpio_set_function(PIN_CS_LED, GPIO_FUNC_SIO);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DATA_CMD, GPIO_OUT);
    gpio_set_dir(PIN_CS_LED, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_DATA_CMD, 0);
    gpio_put(PIN_CS_LED, 1);

    // Initialize LED strips with sample GRB values
    for (int i = 0; i < 60; i++) {
        ws_set_pixel(led_strip0, i, 0x0F, 0x00, 0x0F); // purple
        ws_set_pixel(led_strip1, i, 0x00, 0x0F, 0x00); // green
        ws_set_pixel(led_strip2, i, 0x0F, 0x00, 0x00); // blue
        ws_set_pixel(led_strip3, i, 0x00, 0x0F, 0x0F); // yellow
    }

    // Start with a black background
    screen_reset();
    sleep_ms(500);
    //480x272 display, so each quadrant is 240x136 pixels
    // Draw a small embedded image at the top-left of the screen
    
    
    
    while (true) {
        
        ws_send_frame(0, led_strip0);
        ws_send_frame(0, led_strip0);
        ws_send_frame(0, led_strip0);
        draw_image(0, 0, 240U, 136U, UPARROW_PIXELS);
        sleep_ms(750);
        fill_window(0, 480, 0, 136, COLOR_BLACK);

        
        ws_send_frame(1, led_strip1);
        ws_send_frame(1, led_strip1);
        ws_send_frame(1, led_strip1);
        draw_image(240, 0, 240U, 136U, DOWNARROW_PIXELS);
        sleep_ms(750);
        fill_window(480, 959, 0, 136, COLOR_BLACK);

        
        ws_send_frame(2, led_strip2);
        ws_send_frame(2, led_strip2);
        ws_send_frame(2, led_strip2);
        draw_image(0, 137, 240U, 136U, IMAGE4_PIXELS);
        sleep_ms(750);
        fill_window(0, 479, 136, 271, COLOR_BLACK);

        
        ws_send_frame(0, led_strip3);
        ws_send_frame(0, led_strip3);
        ws_send_frame(0, led_strip3);
        draw_image(240, 137, 240U, 136U, IMAGE2_PIXELS);
        sleep_ms(750);
        fill_window(480, 959, 136, 271, COLOR_BLACK);
    }
}


