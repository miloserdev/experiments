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

#define __DEBUG__ 1

#include "msx_debug.c"
#include "msx_event_loop.c"
#include "msx_uart.c"
#include "msx_wifi.c"
#include "msx_ota.c"
#include "msx_espnow.c"
#include "msx_utils.c"


char to_send[] = "[{\"light\":\"\"}]";
bool pp = true;

void app_loop()
{
    vTaskDelay(3000 / portTICK_RATE_MS);

/* 
    uint8_t buf[200];
    send_packet_raw(broadcast_mac, buf, sizeof(buf));
*/

    if (pp)
    {
        __MSX_DEBUG__( radar_peers() );
        __MSX_DEBUGV__( print_peers() );
        pp = false;
    }

    size_t sz = strlen(to_send);
    packet_t *pack = os_malloc(sizeof(packet_t));
    pack->magic = esp_random();
    pack->type = PACKET_TYPE_DATA;
    memcpy(pack->buffer, to_send, sz);
    pack->len = sz;

    __MSX_DEBUG__( multi_cast(pack) );

    __MSX_DEBUGV__( os_free(pack) );

    os_printf("esp_get_free_heap_size >> %d \n", esp_get_free_heap_size());

    return;
}


void app_main()
{
    __MSX_DEBUG__( nvs_flash_init() );

    // FIX >> bad receive with espnow >> APSTA STA

    __MSX_DEBUG__( init_event_loop() );
    __MSX_DEBUG__( init_uart() );
    __MSX_DEBUG__( init_wifi(WIFI_MODE_APSTA) );
    //__MSX_DEBUG__( setup_wifi(ESP_IF_WIFI_AP, (uint8_t*) "ESP", (uint8_t*) "nullnullnull119911", WIFI_PS_NONE) );
    __MSX_DEBUG__( setup_wifi(ESP_IF_WIFI_STA, (uint8_t*) MESH_SSID_PREFIX, (uint8_t*) MESH_PASSWD, WIFI_PS_NONE) );
    __MSX_DEBUG__( init_httpd() );
    __MSX_DEBUG__( init_ota() );
    __MSX_DEBUG__( init_espnow() );
    __MSX_DEBUG__( init_user_loop(app_loop) );

    init_gpio(GPIO_NUM_12, GPIO_MODE_INPUT);
    init_gpio(GPIO_NUM_2, GPIO_MODE_OUTPUT); // LED

    os_printf("MAIN INIT DONE \n\n");

}
