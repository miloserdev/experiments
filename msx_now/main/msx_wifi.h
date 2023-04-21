#ifndef __MSX_WIFI_INIT__
#define __MSX_WIFI_INIT__


#include <FreeRTOS.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_now.h>


#include "msx_debug.h"
#include "msx_event_loop.h"


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
#define MESH_RECONNECT	true
#define MESH_RECONNECT_ATTEMPTS 10
extern uint32_t reconnect_attempts;

extern esp_interface_t WIFI_IF;
extern uint8_t my_mac[ESP_NOW_ETH_ALEN];

/*
    and yes, i show you my credentials, good luck to find me XD
*/
#define MESH_SSID_PREFIX            "Keenetic-6193"
#define MESH_SSID_PREFIX_LEN        strlen(MESH_SSID_PREFIX)
#define MESH_PASSWD                 "DNj6KdZT"
#define MESH_PASSWD_LEN             ets_strlen(MESH_PASSWD)
#define MESH_CHANNEL                0 // CONFIG_ESPNOW_CHANNEL
#define MESH_MAX_HOP                (4)


esp_err_t init_wifi(wifi_mode_t mode);
esp_err_t setup_wifi(esp_interface_t ifidx, uint8_t ssid[32], uint8_t password[64], wifi_ps_type_t power);
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static wifi_scan_config_t scan_config  = {
	.ssid = NULL,
	.bssid = NULL,
	.channel = 0,
	.show_hidden = true
};

void scan_start_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void scan_done_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

esp_err_t set_wifi_power(int8_t dB);

#endif