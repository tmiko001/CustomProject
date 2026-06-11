#include "lcd_helpers.h"

static void spi_write_byte(uint8_t byte) {
    spi_write_blocking(SPI_PORT, &byte, 1);
}

static void send_command(uint8_t cmd, const uint8_t *params, int param_count) {
    gpio_put(PIN_DATA_CMD, 0);
    gpio_put(PIN_CS, 0);
    spi_write_byte(cmd);
    for (int i = 0; i < param_count; i++) spi_write_byte(params[i]);
    gpio_put(PIN_CS, 1);
}

static void send_pixel_byte(uint8_t byte) {
    gpio_put(PIN_CS, 0);
    spi_write_byte(byte);
    gpio_put(PIN_CS, 1);
}

static void send_pixel_block(const uint8_t *data, int count) {
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, data, count);
    gpio_put(PIN_CS, 1);
}

void fill_window(uint16_t col_start, uint16_t col_end, uint16_t row_start, uint16_t row_end, uint16_t color) {
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

void draw_image(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *pixels) {
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

void screen_reset(void) {
    fill_window(0, 959, 0, 271, COLOR_BLACK);
}

void set_quadrant(uint16_t color, int quadrant) {
    uint16_t col_start, col_end, row_start, row_end;

    if (quadrant == QUAD_TL) {
        col_start = 0; col_end = 479; row_start = 0; row_end = 135;
    } else if (quadrant == QUAD_TR) {
        col_start = 480; col_end = 959; row_start = 0; row_end = 135;
    } else if (quadrant == QUAD_BL) {
        col_start = 0; col_end = 479; row_start = 136; row_end = 271;
    } else { // QUAD_BR
        col_start = 480; col_end = 959; row_start = 136; row_end = 271;
    }

    fill_window(col_start, col_end, row_start, row_end, color);
}