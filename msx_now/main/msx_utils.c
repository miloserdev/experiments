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
esp_err_t setup_gpio(gpio_num_t _pin, gpio_mode_t mode);
void blink();


#define ON_VAL "on"
#define OFF_VAL "off"
char *parse_value(int value, bool invert)
{
    return ( char* ) ( ( value ^ invert ) ? ON_VAL : OFF_VAL );
}


void blink()
{
    gpio_set_level(GPIO_NUM_2, 0x1);
    vTaskDelay(50 / portTICK_RATE_MS);
    gpio_set_level(GPIO_NUM_2, 0x0);
}


cJSON * read_pin(int pin)
{
    char pin_name[10] = "0";
    sprintf(pin_name, "%d", pin);

    int val1 = gpio_get_level(pin);
    char *ret_ = parse_value(val1, false);
    cJSON *pin1 = cJSON_CreateObject();
    cJSON_AddStringToObject(pin1, "pin", pin_name);
    cJSON_AddStringToObject(pin1, "value", ret_);
    return pin1;
}


esp_err_t init_gpio(gpio_num_t _pin, gpio_mode_t mode)
{
    gpio_config_t config = {
        .pin_bit_mask   = (BIT(_pin)),
        .mode           = mode,
        .intr_type      = GPIO_INTR_ANYEDGE,
/*         .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en   = GPIO_PULLDOWN_ENABLE, */
    };
    return gpio_config(&config);
}


#endif