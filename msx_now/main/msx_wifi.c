#ifndef __MSX_WIFI_INIT__
#define __MSX_WIFI_INIT__


#include <FreeRTOS.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_now.h>

#include "msx_debug.c"
#include "msx_event_loop.c"


#if CONFIG_STATION_MODE
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define MESH_SSID_PREFIX            "Keenetic-6193"
#define MESH_SSID_PREFIX_LEN        ets_strlen(MESH_SSID_PREFIX)
#define MESH_PASSWD                 "DNj6KdZT"
#define MESH_PASSWD_LEN             ets_strlen(MESH_PASSWD)
#define MESH_CHANNEL                0 // CONFIG_ESPNOW_CHANNEL
#define MESH_MAX_HOP                (4)

uint8_t my_mac[ESP_NOW_ETH_ALEN];
//static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


esp_err_t init_wifi(void);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


esp_err_t init_wifi(void)
{
    tcpip_adapter_init();
    __MSX_DEBUG__( esp_event_loop_create_default() );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //wifi_sta_config_t sta = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MESH_SSID_PREFIX,
            .password = MESH_PASSWD,
            .channel = MESH_CHANNEL
        },
    };
    __MSX_DEBUG__( esp_wifi_init(&cfg) );
    __MSX_DEBUG__( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    __MSX_DEBUG__( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    __MSX_DEBUG__( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    __MSX_DEBUG__( esp_wifi_set_mode(WIFI_MODE_APSTA) );
    __MSX_DEBUG__( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    __MSX_DEBUG__( esp_wifi_start() );
    __MSX_DEBUG__( esp_wifi_set_channel(MESH_CHANNEL, 0)) ;
    __MSX_DEBUG__( esp_wifi_connect() );
    __MSX_DEBUG__( esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac) );
    __MSX_DEBUG__( esp_wifi_set_ps(WIFI_PS_NONE) );

    __MSX_DEBUG__( esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac) );

    __MSX_DEBUG__( raise_event(MSX_WIFI_EVENT_WIFI_INIT, NULL, 0, NULL, 0) );

/*     msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->id = MSX_WIFI_EVENT_WIFI_INIT;
    evt->base = NULL;
    evt->data = NULL;
    __MSX_DEBUG__( ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) ); */

    __MSX_DEBUGV__( os_free(&cfg) );
    __MSX_DEBUGV__( os_free(&wifi_config) );

    return ESP_OK;
}


void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    __MSX_DEBUG__( raise_event(event_id, event_base, 0, event_data, 0) );
}


#endif