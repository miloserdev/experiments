#define MESH_PASSWD             "DNj6KdZT"
#define MESH_PASSWD_LEN         ets_strlen(MESH_PASSWD)
#define MESH_MAX_HOP            (4)
#define MESH_SSID_PREFIX        "Keenetic-6193"
#define MESH_SSID_PREFIX_LEN    ets_strlen(MESH_SSID_PREFIX)
#define MESH_CHANNEL            0 // CONFIG_ESPNOW_CHANNEL

#define PORT        8066


static const char *TAG = "espnow_example";
uint8_t my_mac[ESP_NOW_ETH_ALEN];
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint8_t pingmsg[128];


#define CONFIG_STATION_MODE     1

#if CONFIG_STATION_MODE
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif