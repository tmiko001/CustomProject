#include "touch_helpers.h"

bool is_touched(void) {
    gpio_init(X_NEG);
    gpio_set_dir(X_NEG, GPIO_OUT);
    gpio_put(X_NEG, 0);

    gpio_init(Y_POS);
    gpio_set_dir(Y_POS, GPIO_IN);
    gpio_pull_up(Y_POS);

    gpio_init(X_POS);
    gpio_set_dir(X_POS, GPIO_IN);
    gpio_disable_pulls(X_POS);
    gpio_init(Y_NEG);
    gpio_set_dir(Y_NEG, GPIO_IN);
    gpio_disable_pulls(Y_NEG);

    sleep_us(10);  
    return !gpio_get(Y_POS);
}

uint16_t read_x(void) {
    gpio_init(X_POS);
    gpio_set_dir(X_POS, GPIO_OUT);
    gpio_put(X_POS, 1);
    gpio_init(X_NEG);
    gpio_set_dir(X_NEG, GPIO_OUT);
    gpio_put(X_NEG, 0);

    gpio_init(Y_NEG);
    gpio_set_dir(Y_NEG, GPIO_IN);
    gpio_disable_pulls(Y_NEG);

    adc_gpio_init(Y_POS);
    adc_select_input(1);  

    sleep_us(10);  
    return adc_read();
}

uint16_t read_y(void) {
    gpio_init(Y_POS);
    gpio_set_dir(Y_POS, GPIO_OUT);
    gpio_put(Y_POS, 1);
    gpio_init(Y_NEG);
    gpio_set_dir(Y_NEG, GPIO_OUT);
    gpio_put(Y_NEG, 0);

    gpio_init(X_NEG);
    gpio_set_dir(X_NEG, GPIO_IN);
    gpio_disable_pulls(X_NEG);

    adc_gpio_init(X_POS);
    adc_select_input(0);  

    sleep_us(10);  
    return adc_read();
}

uint16_t get_smoothed_x(void) {
    uint32_t sum = 0;
    const int samples = 10;
    for(int i = 0; i < samples; i++) {
        sum += read_x(); 
    }
    return sum / samples;
}

uint16_t get_smoothed_y(void) {
    uint32_t sum = 0;
    const int samples = 10;
    for(int i = 0; i < samples; i++) {
        sum += read_y(); 
    }
    return sum / samples;
}

int32_t map_val(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void touch_init(void) {
    adc_init();
}