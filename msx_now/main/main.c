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


void app_loop()
{

    vTaskDelay(2000 / portTICK_RATE_MS);

    uint8_t broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    uint8_t buf[200];
    send_packet_raw(broadcast_mac, buf, sizeof(buf));

    __MSX_PRINTF__("esp_get_free_heap_size >> %d \n", esp_get_free_heap_size());

    return;
}


/* 
E (13260) esp-tls-mbedtls: No server verification option set in esp_tls_cfg_t strue
E (13263) esp-tls-mbedtls: Failed to set client configurations
E (13269) esp-tls: create_ssl_handle failed
E (13274) esp-tls: Failed to open new connection
E (13279) TRANS_SSL: Failed to open a new connection
E (13287) HTTP_CLIENT: Connection failed, sock < 0
E (13291) esp_https_ota: Failed to open HTTP connection: 28674
H: 44536 | L: -392 | http_handle_ota_from_git >>> update failed! 
H: 44276 | L: -260 | app_loop >>> esp_get_free_heap_size >> 44276 
*/

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