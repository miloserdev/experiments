#ifndef __MSX_WIFI_INIT__
#define __MSX_WIFI_INIT__


#include <FreeRTOS.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_now.h>

#include "msx_debug.c"
#include "msx_event_loop.c"


#define MESH_WIFI_STA 0
//MESX

#if MESH_WIFI_STA
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define MESH_MY_PREFIX  "ESP"
#define MESH_MY_PREFIX_LEN  strlen(MESH_MY_PREFIX)
#define MESH_MY_PASSWD  "9857wuj9d9jsie"

#define MESH_SSID_PREFIX            "Keenetic-6193"
#define MESH_SSID_PREFIX_LEN        strlen(MESH_SSID_PREFIX)
#define MESH_PASSWD                 "DNj6KdZT"
#define MESH_PASSWD_LEN             ets_strlen(MESH_PASSWD)
#define MESH_CHANNEL                0 // CONFIG_ESPNOW_CHANNEL
#define MESH_MAX_HOP                (4)

esp_interface_t WIFI_IF = ESP_IF_MAX;
uint8_t my_mac[ESP_NOW_ETH_ALEN];
//static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


esp_err_t init_wifi(wifi_mode_t mode);
esp_err_t setup_wifi(esp_interface_t ifidx, uint8_t ssid[32], uint8_t password[64], wifi_ps_type_t power);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void scan_start_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void scan_done_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


esp_err_t init_wifi(wifi_mode_t mode)
{
    tcpip_adapter_init();
    __MSX_DEBUG__( esp_event_loop_create_default() );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //wifi_sta_config_t sta = WIFI_INIT_CONFIG_DEFAULT();
    __MSX_DEBUG__( esp_wifi_init(&cfg) );
    __MSX_DEBUG__( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    __MSX_DEBUG__( esp_wifi_set_mode(mode) );
    __MSX_DEBUG__( esp_wifi_start() );

    __MSX_DEBUG__( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    __MSX_DEBUG__( esp_event_handler_register(WIFI_EVENT, SYSTEM_EVENT_SCAN_DONE, &scan_done_handler, NULL) );
    __MSX_DEBUG__( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    __MSX_DEBUG__( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &scan_start_handler, NULL) );

    __MSX_DEBUG__( esp_wifi_set_channel(MESH_CHANNEL, 0)) ;

    __MSX_DEBUG__( raise_event(MSX_WIFI_EVENT_WIFI_INIT, NULL, 0, NULL, 0) );

/*     msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->id = MSX_WIFI_EVENT_WIFI_INIT;
    evt->base = NULL;
    evt->data = NULL;
    __MSX_DEBUG__( ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) ); */

    __MSX_DEBUGV__( os_free(&cfg) );

    return ESP_OK;
}

esp_err_t setup_wifi(esp_interface_t ifidx, uint8_t ssid[32], uint8_t password[64], wifi_ps_type_t power)
{
    WIFI_IF = ifidx;
    wifi_config_t wifi_config =
    (ifidx == ESP_IF_WIFI_AP)
    ? (wifi_config_t) {
        .ap = {
            .ssid = MESH_MY_PREFIX,
            .ssid_len = MESH_MY_PREFIX_LEN,
            .password = MESH_MY_PASSWD,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = MESH_CHANNEL,
        }
    }
    : (wifi_config_t) {
        .sta = {
            .ssid = MESH_SSID_PREFIX,
            .password = MESH_PASSWD,
            .channel = MESH_CHANNEL,
        },
    };

    __MSX_DEBUG__( esp_wifi_set_config(ifidx, &wifi_config) );
    __MSX_DEBUG__( esp_wifi_connect() );
    __MSX_DEBUG__( esp_wifi_get_mac(ifidx, my_mac) );
    __MSX_DEBUG__( esp_wifi_set_ps(power) );
    __MSX_DEBUG__( esp_wifi_get_mac(ifidx, my_mac) );

    return ESP_OK;
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    __MSX_DEBUG__( raise_event(event_id, event_base, 0, event_data, 0) );
}


static wifi_scan_config_t config  = {
	.ssid = NULL,
	.bssid = NULL,
	.channel = 0,
	.show_hidden = true
};


void scan_start_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    __MSX_DEBUG__( esp_wifi_scan_start(&config, true) );
}


void scan_done_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t err = ESP_OK;

    uint16_t size = 0;
    wifi_ap_record_t *record;
    
    esp_wifi_scan_get_ap_num(&size);

    record = (wifi_ap_record_t *) os_malloc(sizeof(wifi_ap_record_t) * size);

    err = esp_wifi_scan_get_ap_records(&size, record);
    if (err != ESP_OK) return err;

    /*  = (wifi_ap_record_t *) os_malloc(sizeof(wifi_ap_record_t)); */

    for (size_t i = 0; i < size; i++)
    {
        wifi_ap_record_t *ap = &record[i];
        if (ap == NULL) return;
        __MSX_PRINTF__("ssid %.*s | bssid "MACSTR"", 33, ap->ssid, MAC2STR(ap->bssid));
    }

    os_free(record);
}


#endif