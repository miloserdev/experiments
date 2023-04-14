/* BSD Socket API Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
/* #include "protocol_examples_common.h" */
#include "nvs.h"
#include "nvs_flash.h"

#include <driver/uart.h>
#include <driver/gpio.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <netinet/in.h>
#include <lwip/inet.h>



#include <esp8266/eagle_soc.h>  // ????????

#define __DEBUG__

#include "msx_debug.c"
#include "msx_event_loop.c"
#include "msx_uart.c"
#include "msx_wifi.c"
#include "msx_ota.c"
#include "msx_espnow.c"
#include "msx_utils.c"


xQueueHandle interputQueue;



/* static const char *TAG = "example"; */

/* void loop_xtask(void* params)
{
    int pinNumber, count = 0;
    while (true)
    {
        if (xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY))
        {
            printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count++, gpio_get_level(GPIO_NUM_12));
            gpio_set_level(GPIO_NUM_2, !(bool)gpio_get_level(GPIO_NUM_2));
        }
    }
} */


/*
//gpio_isr_t
static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}
*/


void app_loop()
{

    vTaskDelay(2000 / portTICK_RATE_MS);

    uint8_t broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    uint8_t buf[200];
    send_packet_raw(broadcast_mac, buf, sizeof(buf));

    return;
}

void app_main()
{
    __MSX_DEBUG__( nvs_flash_init() );


    __MSX_DEBUG__( init_event_loop() );
    __MSX_DEBUG__( init_uart() );
    __MSX_DEBUG__( init_wifi(WIFI_MODE_STA) );
    //__MSX_DEBUG__( setup_wifi(ESP_IF_WIFI_AP, (uint8_t*) "ESP", (uint8_t*) "nullnullnull119911", WIFI_PS_NONE) );
    __MSX_DEBUG__( setup_wifi(ESP_IF_WIFI_STA, (uint8_t*) MESH_SSID_PREFIX, (uint8_t*) MESH_PASSWD, WIFI_PS_NONE) );
    __MSX_DEBUG__( init_httpd() );
    __MSX_DEBUG__( init_ota() );
    __MSX_DEBUG__( init_espnow() );
    __MSX_DEBUG__( init_user_loop(app_loop) );

/*     interputQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(loop_xtask, "loop_xtask", 4096, NULL, 1, NULL);
    os_printf("TASKS INIT DONE \n\n"); */

    init_gpio(GPIO_NUM_12, GPIO_MODE_INPUT);
    init_gpio(GPIO_NUM_2, GPIO_MODE_OUTPUT); // LED
    os_printf("GPIOS INIT DONE \n\n");

/* 
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_12, gpio_interrupt_handler, (void*) GPIO_NUM_12);
 */
    os_printf("MAIN INIT DONE \n\n");

}