#ifndef __MSX_UTILS_INIT__
#define __MSX_UTILS_INIT__


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>

#include <driver/gpio.h>
#include <sys/param.h>

#include <cJSON.h>
/* #define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y) */



char *parse_value(int value, bool invert);
cJSON * read_pin(int pin);
esp_err_t init_gpio(gpio_num_t _pin, gpio_mode_t mode);
void blink();


#define ON_VAL "on"
#define OFF_VAL "off"

#endif